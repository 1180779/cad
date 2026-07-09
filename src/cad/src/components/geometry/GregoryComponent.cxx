//
// Created by Radosław Głasek on 09.07.2026
//

#include "GregoryComponent.hxx"

#include <algorithm>
#include <utility>

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
    m_vectorsEbo.deleteGpu(gl);
    m_patchVao.deleteGpu(gl);
    m_vectorsVao.deleteGpu(gl);
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
    m_gridDivisionsU.resize(netCount(), 4);
    m_gridDivisionsV.resize(netCount(), 4);

    std::vector<uint32_t> indices(netCount() * gregory::Net::pointCount);
    std::ranges::iota(indices, 0u);
    m_ebo.assign(std::move(indices));

    // ring segments + cross-tangent vectors (anchor -> interior candidate),
    // per net, indexing the shared 20-per-net VBO
    static constexpr std::array<std::pair<uint32_t, uint32_t>, 20> netLines{
        {
            // ring A -> B -> C -> D -> A
            {0, 1},
            {1, 2},
            {2, 3},
            {3, 4},
            {4, 5},
            {5, 6},
            {6, 7},
            {7, 8},
            {8, 9},
            {9, 10},
            {10, 11},
            {11, 0},
            // continuity vectors
            // f00u, f00v (corner A)
            {1, 12},
            {11, 13},
            // f10u, f10v (corner B)
            {2, 14},
            {4, 15},
            // f11u, f11v (corner C)
            {7, 16},
            {5, 17},
            // f01u, f01v (corner D)
            {8, 18},
            {10, 19},
        }
    };
    std::vector<uint32_t> lines;
    lines.reserve(netCount() * netLines.size() * 2);
    for (int i = 0; i < netCount(); ++i) {
        const uint32_t base = i * gregory::Net::pointCount;
        for (const auto &[a, b] : netLines) {
            lines.push_back(base + a);
            lines.push_back(base + b);
        }
    }
    m_vectorsEbo.assign(std::move(lines));
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

void GregoryComponent::setGridDivisionsU(const int net, const int divisions) {
    if (net < 0 || net >= m_gridDivisionsU.size()) {
        return;
    }
    m_gridDivisionsU[net] = std::max(1, divisions);
}

void GregoryComponent::setGridDivisionsV(const int net, const int divisions) {
    if (net < 0 || net >= m_gridDivisionsV.size()) {
        return;
    }
    m_gridDivisionsV[net] = std::max(1, divisions);
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
    m_vectorsEbo.syncToGpu(gl);
    if (!m_patchVao.created() && !m_ebo.empty()) {
        m_patchVao.setup(gl, m_vbo.vboId(), m_ebo.vboId());
    }
    if (!m_vectorsVao.created() && !m_vectorsEbo.empty()) {
        m_vectorsVao.setup(gl, m_vbo.vboId(), m_vectorsEbo.vboId());
    }
}
