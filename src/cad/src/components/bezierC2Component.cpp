//
// Created on 5/5/26.
//

#include "bezierC2Component.hpp"

#include <algorithm>
#include <ranges>
#include <span>

#include "GlCommon.hpp"

BezierC2Component::BezierC2Component(PointRegistry *registry) : m_registry{registry} {
    m_positionCallbackId = registry->subscribeToPositionChanges(
        [this](const PointHandle h) {
            if (std::ranges::find(m_deBoorPoints, h) != m_deBoorPoints.end()) {
                m_bernsteinDirty = true;
                m_needsUpdate = true;
            }
        }
    );
}

BezierC2Component::~BezierC2Component() {
    for (const auto &id : m_removeControlPointCallbacks | std::views::values) { m_registry->unsubscribeFromRemove(id); }

    if (m_positionCallbackId >= 0) { m_registry->unsubscribeFromPositionChanges(m_positionCallbackId); }

    const auto gl = GL();
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
        [this, handle](const PointHandle removed) { if (removed == handle) { removeControlPoint(handle); } }
    );
    m_removeControlPointCallbacks[handle] = id;

    m_needsUpdate = true;
    m_bernsteinDirty = true;
}

void BezierC2Component::removeControlPointAt(const int index) {
    if (index < 0 || index >= static_cast<int>(m_deBoorPoints.size())) { return; }
    const PointHandle h = m_deBoorPoints[index];
    removeAssociatedCallback(h);
    m_deBoorPoints.erase(m_deBoorPoints.begin() + index);
    m_needsUpdate = true;
    m_bernsteinDirty = true;
}

void BezierC2Component::removeControlPoint(const PointHandle handle) {
    const auto it = std::ranges::find(m_deBoorPoints, handle);
    if (it == m_deBoorPoints.end()) { return; }
    removeAssociatedCallback(handle);
    m_deBoorPoints.erase(it);
    m_needsUpdate = true;
    m_bernsteinDirty = true;
}

void BezierC2Component::setShowDeBoorPolygon(const bool v) {
    m_showDeBoorPolygon = v;
    m_needsUpdate = true;
}

void BezierC2Component::setParametrizationMode(const ParametrizationMode mode) {
    if (m_parametrizationMode == mode) { return; }
    m_parametrizationMode = mode;
    m_bernsteinDirty = true;
    m_needsUpdate = true;
}

void BezierC2Component::setBernsteinPosition(const int bernsteinIndex, const cadm::vec3 newPos) {
    const int local = bernsteinIndex % 4;
    const int segment = bernsteinIndex / 4;

    // All inverses use the uniform-knot formula. In chord-length mode the result is
    // approximate (the de Boor point shifts, the curve recomputes with new chord lengths),
    // but it gives meaningful interactive control.
    if (local == 0) {
        // b0 = (d0 + 4*d1 + d2)/6  →  d0 = 6*b0 - 4*d1 - d2
        const cadm::vec3 d1 = m_registry->getPosition(m_deBoorPoints[segment + 1]);
        const cadm::vec3 d2 = m_registry->getPosition(m_deBoorPoints[segment + 2]);
        m_registry->setPosition(m_deBoorPoints[segment], newPos * 6.0f - d1 * 4.0f - d2);
    }
    else if (local == 1) {
        // b1 = (2*d1 + d2)/3  →  d1 = (3*b1 - d2) / 2
        const cadm::vec3 d2 = m_registry->getPosition(m_deBoorPoints[segment + 2]);
        m_registry->setPosition(m_deBoorPoints[segment + 1], (newPos * 3.0f - d2) * 0.5f);
    }
    else if (local == 2) {
        // b2 = (d1 + 2*d2)/3  →  d2 = (3*b2 - d1) / 2
        const cadm::vec3 d1 = m_registry->getPosition(m_deBoorPoints[segment + 1]);
        m_registry->setPosition(m_deBoorPoints[segment + 2], (newPos * 3.0f - d1) * 0.5f);
    }
    else // local == 3
    {
        // b3 = (d1 + 4*d2 + d3)/6  →  d3 = 6*b3 - d1 - 4*d2
        const cadm::vec3 d1 = m_registry->getPosition(m_deBoorPoints[segment + 1]);
        const cadm::vec3 d2 = m_registry->getPosition(m_deBoorPoints[segment + 2]);
        m_registry->setPosition(m_deBoorPoints[segment + 3], newPos * 6.0f - d1 - d2 * 4.0f);
    }

    m_bernsteinDirty = true;
    m_needsUpdate = true;
}

void BezierC2Component::setShowBernsteinPolygon(const bool v) {
    m_showBernsteinPolygon = v;
    m_needsUpdate = true;
}

int BezierC2Component::segmentCount() const {
    const int n = static_cast<int>(m_deBoorPoints.size());
    return n >= 4
               ? n - 3
               : 0;
}

void BezierC2Component::regenerateMesh() {
    m_needsUpdate = true;
    m_bernsteinDirty = true;
}

void BezierC2Component::removeAssociatedCallback(const PointHandle h) {
    const auto it = m_removeControlPointCallbacks.find(h);
    if (it == m_removeControlPointCallbacks.end()) { return; }
    m_registry->unsubscribeFromRemove(it->second);
    m_removeControlPointCallbacks.erase(it);
}

void BezierC2Component::recomputeBernstein() {
    bsplineToBezier::convert(
        m_parametrizationMode,
        std::span<const PointHandle>(m_deBoorPoints),
        *m_registry,
        m_bernsteinPositions
    );

    m_bernsteinVbo.assign(m_bernsteinPositions);

    // Sequential patch EBO: 0,1,2,3, 4,5,6,7, ...
    const int totalBernstein = static_cast<int>(m_bernsteinPositions.size());
    std::vector<uint32_t> patchIdx(static_cast<size_t>(totalBernstein));
    for (int i = 0; i < totalBernstein; ++i) { patchIdx[i] = static_cast<uint32_t>(i); }
    m_patchEbo.assign(std::move(patchIdx));

    // De Boor EBO: PointHandle values (slot indices) for line strip through de Boor points
    const int n = static_cast<int>(m_deBoorPoints.size());
    std::vector<uint32_t> deBoorIdx(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) { deBoorIdx[i] = m_deBoorPoints[i]; }
    m_deBoorEbo.assign(std::move(deBoorIdx));
}

void BezierC2Component::syncToGpu() {
    if (m_bernsteinDirty) {
        recomputeBernstein();
        m_bernsteinDirty = false;
    }

    const auto gl = GL();
    m_bernsteinVbo.syncToGpu(gl);
    m_patchEbo.syncToGpu(gl);
    m_deBoorEbo.syncToGpu(gl);

    if (m_patchVao == 0 && segmentCount() > 0) { setupPatchVao(gl); }
    if (m_bernsteinPolyVao == 0 && segmentCount() > 0) { setupBernsteinPolyVao(gl); }
    if (m_deBoorVao == 0 && m_deBoorPoints.size() >= 2) { setupDeBoorVao(gl); }

    m_needsUpdate = false;
}

void BezierC2Component::setupPatchVao(QOpenGLFunctions_4_5_Core *gl) {
    gl->glGenVertexArrays(1, &m_patchVao);
    gl->glBindVertexArray(m_patchVao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_bernsteinVbo.vboId());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_CADM_VT_TYPE, GL_FALSE, 3 * GL_CADM_VT_SIZE, nullptr);
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
    gl->glVertexAttribPointer(0, 3, GL_CADM_VT_TYPE, GL_FALSE, 3 * GL_CADM_VT_SIZE, nullptr);
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
    gl->glVertexAttribPointer(0, 3, GL_CADM_VT_TYPE, GL_FALSE, 3 * GL_CADM_VT_SIZE, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_deBoorEbo.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
