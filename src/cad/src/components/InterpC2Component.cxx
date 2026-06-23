//
// Created by Radosław Głasek on 23.06.2026
//

#include "InterpC2Component.hxx"

#include <algorithm>
#include <ranges>
#include <span>

#include "../InterpC2Solver.hxx"
#include "GlCommon.hpp"

InterpC2Component::InterpC2Component(PointRegistry *registry) : m_registry{registry} {
    m_positionCallbackId = registry->subscribeToPositionChanges(
        [this](const PointHandle h) {
            if (std::ranges::find(m_points, h) != m_points.end()) {
                markDirty();
            }
        }
    );
}

InterpC2Component::~InterpC2Component() {
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
    if (m_polylineVao) {
        gl->glDeleteVertexArrays(1, &m_polylineVao);
        m_polylineVao = 0;
    }
    if (m_bernsteinPolyVao) {
        gl->glDeleteVertexArrays(1, &m_bernsteinPolyVao);
        m_bernsteinPolyVao = 0;
    }
    m_bernsteinVbo.deleteGpu(gl);
    m_patchEbo.deleteGpu(gl);
    m_polylineEbo.deleteGpu(gl);
}

void InterpC2Component::addControlPoint(const PointHandle handle) {
    m_points.push_back(handle);

    const CallbackId id = m_registry->subscribeToRemove(
        [this, handle](const PointHandle removed) {
            if (removed == handle) {
                removeControlPoint(handle);
            }
        }
    );
    m_removeControlPointCallbacks[handle] = id;

    markDirty();
}

void InterpC2Component::removeControlPointAt(const int index) {
    if (index < 0 || index >= static_cast<int>(m_points.size())) {
        return;
    }
    removeAssociatedCallback(m_points[index]);
    m_points.erase(m_points.begin() + index);
    markDirty();
}

void InterpC2Component::removeControlPoint(const PointHandle handle) {
    const auto it = std::ranges::find(m_points, handle);
    if (it == m_points.end()) {
        return;
    }
    removeAssociatedCallback(handle);
    m_points.erase(it);
    markDirty();
}

void InterpC2Component::setShowControlPolyline(const bool v) {
    if (m_showPolyline != v) {
        m_showPolyline = v;
        if (v) {
            m_needsUpdate = true;
        }
    }
}

void InterpC2Component::setShowBernsteinPolygon(const bool v) {
    if (m_showBernsteinPolygon != v) {
        m_showBernsteinPolygon = v;
        if (v) {
            m_needsUpdate = true;
        }
    }
}

void InterpC2Component::setShowBernsteinCps(const bool v) {
    m_showBernsteinCps = v;
}

void InterpC2Component::regenerateMesh() {
    markDirty();
}

void InterpC2Component::markDirty() {
    m_dirty = true;
    m_needsUpdate = true;
}

void InterpC2Component::removeAssociatedCallback(const PointHandle h) {
    const auto it = m_removeControlPointCallbacks.find(h);
    if (it == m_removeControlPointCallbacks.end()) {
        return;
    }
    m_registry->unsubscribeFromRemove(it->second);
    m_removeControlPointCallbacks.erase(it);
}

void InterpC2Component::ensureBernsteinUpToDate() {
    if (m_dirty) {
        recompute();
    }
}

void InterpC2Component::recompute() {
    auto positions = m_points
        | std::views::transform(
            [this](auto h) {
                return m_registry->getPosition(h);
            }
        )
        | std::ranges::to<std::vector>();

    std::vector<cadm::Vec3> bernstein;
    interpC2::solve(std::span<const cadm::Vec3>(positions), bernstein);

    const std::size_t segs = bernstein.empty()
                                 ? 0
                                 : (bernstein.size() - 1) / 3;

    std::vector<uint32_t> patchIdx(static_cast<std::size_t>(segs) * 4);
    for (std::size_t s = 0; s < segs; ++s) {
        const auto base = static_cast<uint32_t>(s * 3);
        patchIdx[s * 4 + 0] = base;
        patchIdx[s * 4 + 1] = base + 1;
        patchIdx[s * 4 + 2] = base + 2;
        patchIdx[s * 4 + 3] = base + 3;
    }

    std::vector<uint32_t> polyIdx = m_points | std::ranges::to<std::vector<uint32_t>>();
    m_bernsteinVbo.diffAssign(std::move(bernstein));
    m_patchEbo.diffAssign(std::move(patchIdx));
    m_polylineEbo.diffAssign(std::move(polyIdx));

    m_dirty = false;
}

void InterpC2Component::syncToGpu() {
    if (m_dirty) {
        recompute();
    }

    const auto gl = getGl();
    m_bernsteinVbo.syncToGpu(gl);
    m_patchEbo.syncToGpu(gl);
    m_polylineEbo.syncToGpu(gl);

    if (m_patchVao == 0 && segmentCount() > 0) {
        setupPatchVao(gl);
    }
    if (m_bernsteinPolyVao == 0 && segmentCount() > 0) {
        setupBernsteinPolyVao(gl);
    }
    if (m_polylineVao == 0 && m_points.size() >= 2) {
        setupPolylineVao(gl);
    }

    m_needsUpdate = false;
}

void InterpC2Component::setupPatchVao(QOpenGLFunctions_4_5_Core *gl) {
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

void InterpC2Component::setupBernsteinPolyVao(QOpenGLFunctions_4_5_Core *gl) {
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

void InterpC2Component::setupPolylineVao(QOpenGLFunctions_4_5_Core *gl) {
    gl->glGenVertexArrays(1, &m_polylineVao);
    gl->glBindVertexArray(m_polylineVao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_registry->getPositionVBO());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, gc_glCadmVtType, GL_FALSE, 3 * gc_glCadmVtSize, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_polylineEbo.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
