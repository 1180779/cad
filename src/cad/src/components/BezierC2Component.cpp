//
// Created on 5/5/26.
//

#include "BezierC2Component.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <ranges>
#include <span>

#include "BSplineToBezierConverter.hpp"
#include "GlCommon.hpp"

BezierC2Component::BezierC2Component(PointRegistry *registry) : m_registry{registry} {
    m_positionCallbackId = registry->subscribeToPositionChanges(
        [this](const PointHandle h) {
            if (const auto it = std::ranges::find(m_deBoorPoints, h);
                it != m_deBoorPoints.end()) {
                markDeBoorDirty(static_cast<int>(std::distance(m_deBoorPoints.begin(), it)));
            }
        }
    );
}

BezierC2Component::~BezierC2Component() {
    for (const auto &id : m_removeControlPointCallbacks | std::views::values) {
        m_registry->unsubscribeFromRemove(id);
    }

    if (m_positionCallbackId >= 0) {
        m_registry->unsubscribeFromPositionChanges(m_positionCallbackId);
    }

    const auto gl = getGl();
    if (m_patchVao) {
        gl->glDeleteVertexArrays(1, &m_patchVao);
        m_patchVao = 0;
    }
    if (m_deBoorVao) {
        gl->glDeleteVertexArrays(1, &m_deBoorVao);
        m_deBoorVao = 0;
    }
    if (m_bernsteinPolyVao) {
        gl->glDeleteVertexArrays(1, &m_bernsteinPolyVao);
        m_bernsteinPolyVao = 0;
    }
    m_bernsteinVbo.deleteGpu(gl);
    m_patchEbo.deleteGpu(gl);
    m_deBoorEbo.deleteGpu(gl);
}

void BezierC2Component::addControlPoint(const PointHandle handle) {
    m_deBoorPoints.push_back(handle);

    const CallbackId id = m_registry->subscribeToRemove(
        [this, handle](const PointHandle removed) {
            if (removed == handle) {
                removeControlPoint(handle);
            }
        }
    );
    m_removeControlPointCallbacks[handle] = id;

    markStructureDirty();
}

void BezierC2Component::removeControlPointAt(const int index) {
    if (index < 0 || index >= static_cast<int>(m_deBoorPoints.size())) {
        return;
    }
    const PointHandle h = m_deBoorPoints[index];
    removeAssociatedCallback(h);
    m_deBoorPoints.erase(m_deBoorPoints.begin() + index);
    markStructureDirty();
}

void BezierC2Component::removeControlPoint(const PointHandle handle) {
    const auto it = std::ranges::find(m_deBoorPoints, handle);
    if (it == m_deBoorPoints.end()) {
        return;
    }
    removeAssociatedCallback(handle);
    m_deBoorPoints.erase(it);
    markStructureDirty();
}

void BezierC2Component::setShowDeBoorPolygon(const bool v) {
    if (m_showDeBoorPolygon != v) {
        m_showDeBoorPolygon = v;
        if (v) {
            m_needsUpdate = true;
        }
    }
}

void BezierC2Component::setBernsteinPosition(const int bernsteinIndex, const cadm::Vec3 newPos) const {
    const int local = bernsteinIndex % 3;
    const int k = bernsteinIndex / 3;

    if (local == 1) {
        // move the closer de Boor point
        // b1 = (2*d[k+1] + d[k+2])/3
        // <->
        // d[k+1] = (3*b1 - d[k+2]) / 2
        const cadm::Vec3 dNext = m_registry->getPosition(m_deBoorPoints[k + 2]);
        m_registry->setPosition(m_deBoorPoints[k + 1], (newPos * 3.0f - dNext) * 0.5f);
    }
    else if (local == 2) {
        // move the closer De Boor point
        // b2 = (d[k+1] + 2*d[k+2])/3
        // <->
        // d[k+2] = (3*b2 - d[k+1]) / 2
        const cadm::Vec3 dPrev = m_registry->getPosition(m_deBoorPoints[k + 1]);
        m_registry->setPosition(m_deBoorPoints[k + 2], (newPos * 3.0f - dPrev) * 0.5f);
    }
    else {
        // move the middle De Boor point
        // joint j = (d[k] + 4*d[k+1] + d[k+2])/6
        // <->
        // d[k+1] = (6*j - d[k] - d[k+2]) / 4
        const cadm::Vec3 dPrev = m_registry->getPosition(m_deBoorPoints[k]);
        const cadm::Vec3 dNext = m_registry->getPosition(m_deBoorPoints[k + 2]);
        m_registry->setPosition(m_deBoorPoints[k + 1], (newPos * 6.0f - dPrev - dNext) * 0.25f);
    }
    // each branch moves one de Boor point via setPosition;
    // change callback already marks the affected segments dirty
}

void BezierC2Component::setShowBernsteinPolygon(const bool v) {
    if (m_showBernsteinPolygon != v) {
        m_showBernsteinPolygon = v;
        if (v) {
            m_needsUpdate = true;
        }
    }
}

void BezierC2Component::setShowBernsteinCps(const bool v) {
    m_showBernsteinCps = v;
}

int BezierC2Component::segmentCount() const {
    const int n = static_cast<int>(m_deBoorPoints.size());
    return n >= 4
               ? n - 3
               : 0;
}

void BezierC2Component::regenerateMesh() {
    markStructureDirty();
}

void BezierC2Component::markStructureDirty() {
    m_structureDirty = true;
    m_needsUpdate = true;
}

void BezierC2Component::markSegmentsDirty(const int firstSeg, const int lastSeg) {
    m_dirtySegLo = std::min(m_dirtySegLo, firstSeg);
    m_dirtySegHi = std::max(m_dirtySegHi, lastSeg);
    m_needsUpdate = true;
}

void BezierC2Component::markDeBoorDirty(const int deBoorIndex) {
    const int segs = segmentCount();
    if (segs <= 0) {
        return;
    }
    // segment s is built only from positions d[s...s+3] -> moving d[j] hits [j-3, j]
    markSegmentsDirty(
        std::max(0, deBoorIndex - 3),
        std::min(segs - 1, deBoorIndex)
    );
}

void BezierC2Component::removeAssociatedCallback(const PointHandle h) {
    const auto it = m_removeControlPointCallbacks.find(h);
    if (it == m_removeControlPointCallbacks.end()) {
        return;
    }
    m_registry->unsubscribeFromRemove(it->second);
    m_removeControlPointCallbacks.erase(it);
}

void BezierC2Component::ensureBernsteinUpToDate() {
    if (hasDirtyBernstein()) {
        recomputeBernstein();
    }
}

void BezierC2Component::recomputeBernstein() {
    const int segs = segmentCount();

    const int expectedPositions = segs > 0
                                      ? segs * 3 + 1
                                      : 0;
    if (!m_structureDirty && m_bernsteinVbo.size() != expectedPositions) {
        m_structureDirty = true;
    }

    if (m_structureDirty) {
        std::vector<cadm::Vec3> positions;
        bsplineToBezier::convert(std::span<const PointHandle>(m_deBoorPoints), *m_registry, positions);

        m_bernsteinVbo.diffAssign(std::move(positions));

        std::vector<uint32_t> patchIdx(static_cast<size_t>(segs) * 4);
        for (int s = 0; s < segs; ++s) {
            const auto base = static_cast<uint32_t>(s * 3);
            patchIdx[static_cast<size_t>(s) * 4 + 0] = base;
            patchIdx[static_cast<size_t>(s) * 4 + 1] = base + 1;
            patchIdx[static_cast<size_t>(s) * 4 + 2] = base + 2;
            patchIdx[static_cast<size_t>(s) * 4 + 3] = base + 3;
        }
        m_patchEbo.diffAssign(std::move(patchIdx));

        const int n = static_cast<int>(m_deBoorPoints.size());
        std::vector<uint32_t> deBoorIdx(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) {
            deBoorIdx[i] = m_deBoorPoints[i];
        }
        m_deBoorEbo.diffAssign(std::move(deBoorIdx));

        m_structureDirty = false;
        m_dirtySegLo = std::numeric_limits<int>::max();
        m_dirtySegHi = -1;
        return;
    }

    // geometry-only: recompute and re-upload just the dirty segments straight into the VBO slots
    const int first = std::max(0, m_dirtySegLo);
    const int last = std::min(segs - 1, m_dirtySegHi);
    for (int g = first; g <= last; ++g) {
        cadm::Vec3 b[4];
        bsplineToBezier::uniformSegment(
            m_registry->getPosition(m_deBoorPoints[g]),
            m_registry->getPosition(m_deBoorPoints[g + 1]),
            m_registry->getPosition(m_deBoorPoints[g + 2]),
            m_registry->getPosition(m_deBoorPoints[g + 3]),
            std::span(b)
        );
        // segment g occupies slots [3g, 3g+1, 3g+2, 3g+3]; the boundary slots 3g and
        // 3g+3 are shared with neighbours but get the same join value, so re-writing
        // them here is consistent
        const int base = 3 * g;
        for (int j = 0; j < 4; ++j) {
            m_bernsteinVbo.set(base + j, b[j]);
        }
    }

    m_dirtySegLo = std::numeric_limits<int>::max();
    m_dirtySegHi = -1;
}

void BezierC2Component::syncToGpu() {
    if (hasDirtyBernstein()) {
        recomputeBernstein();
    }

    const auto gl = getGl();
    m_bernsteinVbo.syncToGpu(gl);
    m_patchEbo.syncToGpu(gl);
    m_deBoorEbo.syncToGpu(gl);

    if (m_patchVao == 0 && segmentCount() > 0) {
        setupPatchVao(gl);
    }
    if (m_bernsteinPolyVao == 0 && segmentCount() > 0) {
        setupBernsteinPolyVao(gl);
    }
    if (m_deBoorVao == 0 && m_deBoorPoints.size() >= 2) {
        setupDeBoorVao(gl);
    }

    m_needsUpdate = false;
}

void BezierC2Component::setupPatchVao(QOpenGLFunctions_4_5_Core *gl) {
    gl->glGenVertexArrays(1, &m_patchVao);
    gl->glBindVertexArray(m_patchVao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_bernsteinVbo.vboId());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, gc_glCadmVtType, GL_FALSE, 3 * gc_glCadmVtSize, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchEbo.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BezierC2Component::setupBernsteinPolyVao(QOpenGLFunctions_4_5_Core *gl) {
    gl->glGenVertexArrays(1, &m_bernsteinPolyVao);
    gl->glBindVertexArray(m_bernsteinPolyVao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_bernsteinVbo.vboId());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, gc_glCadmVtType, GL_FALSE, 3 * gc_glCadmVtSize, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchEbo.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BezierC2Component::setupDeBoorVao(QOpenGLFunctions_4_5_Core *gl) {
    gl->glGenVertexArrays(1, &m_deBoorVao);
    gl->glBindVertexArray(m_deBoorVao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_registry->getPositionVBO());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, gc_glCadmVtType, GL_FALSE, 3 * gc_glCadmVtSize, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_deBoorEbo.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
