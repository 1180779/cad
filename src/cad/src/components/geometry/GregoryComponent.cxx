//
// Created by Radosław Głasek on 09.07.2026
//

#include "GregoryComponent.hxx"

#include <algorithm>

#include "GlCommon.hpp"

GregoryComponent::GregoryComponent(PointRegistry *registry)
: m_registry(registry) {
    m_positionCallbackId = registry->subscribeToPositionChanges(
        [this](const PointHandle h) {
            if (std::ranges::contains(m_handles, h)) {
                markForUpdate();
            }
        }
    );
}

GregoryComponent::~GregoryComponent() {
    const auto gl = getGl();
    m_vbo.deleteGpu(gl);
    m_ebo.deleteGpu(gl);
    m_patchVao.deleteGpu(gl);
    m_registry->unsubscribeFromPositionChanges(m_positionCallbackId);
    for (const PointHandle h : m_handles) {
        m_registry->unlock(h);
    }
}

void GregoryComponent::setHole(std::vector<PointHandle> handles) {
    for (const PointHandle h : m_handles) {
        m_registry->unlock(h);
    }
    for (const PointHandle h : handles) {
        m_registry->lock(h);
    }
    m_handles = std::move(handles);

    std::vector<uint32_t> indices(netCount() * gregory::Net::pointCount);
    std::ranges::iota(indices, 0u);
    m_ebo.assign(std::move(indices));
    markForUpdate();
}

void GregoryComponent::replaceControlPoint(const PointHandle from, const PointHandle to) {
    bool found = false;
    for (PointHandle &h : m_handles) {
        if (h != from) {
            continue;
        }
        m_registry->unlock(from);
        m_registry->lock(to);
        h = to;
        found = true;
    }
    if (found) {
        markForUpdate();
    }
}

void GregoryComponent::setControlPointHandles(const std::vector<PointHandle> &handles) {
    if (handles.size() != m_handles.size()) {
        return;
    }
    setHole(handles);
}

void GregoryComponent::setGridDivisionsU(const int divisions) {
    m_gridDivisionsU = std::max(1, divisions);
}

void GregoryComponent::setGridDivisionsV(const int divisions) {
    m_gridDivisionsV = std::max(1, divisions);
}

std::vector<gregory::EdgeData> GregoryComponent::edgeData() const {
    std::vector<gregory::EdgeData> edges(netCount());
    for (int i = 0; i < netCount(); ++i) {
        auto &[boundary, inner] = edges[i];
        const PointHandle *h = m_handles.data() + i * s_handlesPerEdge;
        for (int k = 0; k < 4; ++k) {
            boundary[k] = m_registry->getPosition(h[k]);
            inner[k] = m_registry->getPosition(h[4 + k]);
        }
    }
    return edges;
}

void GregoryComponent::regenerateMesh() {
    const auto nets = gregory::fillHole(edgeData());
    std::vector<cadm::Vec3> vertices;
    vertices.reserve(nets.size() * gregory::Net::pointCount);
    for (const auto &[net] : nets) {
        vertices.insert(vertices.end(), net.begin(), net.end());
    }
    m_vbo.assign(std::move(vertices));
}

void GregoryComponent::syncToGpu() {
    const auto gl = getGl();
    m_vbo.syncToGpu(gl);
    m_ebo.syncToGpu(gl);
    if (!m_patchVao.created() && !m_ebo.empty()) {
        m_patchVao.setup(gl, m_vbo.vboId(), m_ebo.vboId());
    }
}
