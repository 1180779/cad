//
// Created by Radosław Głasek on 02.07.2026
//

#include "PatchComponent.hxx"

#include <algorithm>

#include "GlCommon.hpp"

PatchComponent::PatchComponent(PointRegistry *registry) : m_registry(registry) {
    m_positionCallbackId = registry->subscribeToPositionChanges(
        [this](const PointHandle h) {
            // any control point of this patch moving invalidates the surface geometry
            if (std::ranges::find(m_controlPoints, h) != m_controlPoints.end()) {
                markForUpdate();
            }
        }
    );
}

PatchComponent::~PatchComponent() {
    const auto gl = getGl();
    m_patchEbo.deleteGpu(gl);
    m_netEbo.deleteGpu(gl);
    m_patchVao.deleteGpu(gl);
    m_netVao.deleteGpu(gl);
    m_registry->unsubscribeFromPositionChanges(m_positionCallbackId);
    for (const PointHandle h : m_controlPoints) {
        m_registry->unlock(h);
    }
}

void PatchComponent::setGrid(
    std::vector<PointHandle> handles,
    const int rows,
    const int cols,
    const bool wrapU,
    const int patchCountX,
    const int patchCountY
) {
    for (const PointHandle h : m_controlPoints) {
        m_registry->unlock(h);
    }
    for (const PointHandle h : handles) {
        m_registry->lock(h);
    }
    m_controlPoints = std::move(handles);
    m_rows = rows;
    m_cols = cols;
    m_wrapU = wrapU;
    m_patchCountX = patchCountX;
    m_patchCountY = patchCountY;
    m_selectedPatches.clear();
    m_needsUpdate = true;
    buildNetEbo();
    rebuildPatchData();
}

int PatchComponent::gridIndex(const int row, const int col) const {
    const int c = m_wrapU
                      ? col % m_cols
                      : col;
    return row * m_cols + c;
}

void PatchComponent::gatherPatch(const int px, const int py, std::array<int, 16> &out) const {
    const int rBase = patchRowBase(py);
    const int cBase = patchColBase(px);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            out[i * 4 + j] = gridIndex(rBase + i, cBase + j);
        }
    }
}

SinglePatchView PatchComponent::singlePatch(const int px, const int py) const {
    std::array<int, 16> grid{};
    gatherPatch(px, py, grid);
    std::array<PointHandle, 16> handles{};
    for (int k = 0; k < 16; ++k) {
        handles[k] = m_controlPoints[grid[k]];
    }
    return handles;
}

void PatchComponent::setPatchSelected(const int index, const bool selected) {
    if (selected) {
        m_selectedPatches.insert(index);
    }
    else {
        m_selectedPatches.erase(index);
    }
}

void PatchComponent::buildNetEbo() {
    std::vector<uint32_t> lines;
    // row-direction segments
    for (int r = 0; r < m_rows; ++r) {
        const int last = m_wrapU
                             ? m_cols
                             : m_cols - 1;
        for (int c = 0; c < last; ++c) {
            lines.push_back(m_controlPoints[gridIndex(r, c)]);
            lines.push_back(m_controlPoints[gridIndex(r, c + 1)]);
        }
    }
    // column-direction segments
    for (int c = 0; c < m_cols; ++c) {
        for (int r = 0; r < m_rows - 1; ++r) {
            lines.push_back(m_controlPoints[gridIndex(r, c)]);
            lines.push_back(m_controlPoints[gridIndex(r + 1, c)]);
        }
    }
    m_netEbo.assign(std::move(lines));
}

void PatchComponent::setGridDivisionsU(const int divisions) {
    m_gridDivisionsU = std::max(1, divisions);
}

void PatchComponent::setGridDivisionsV(const int divisions) {
    m_gridDivisionsV = std::max(1, divisions);
}

void PatchComponent::setShowNet(const bool v) {
    m_showNet = v;
}

void PatchComponent::syncToGpu() {
    const auto gl = getGl();
    syncPatchVertices(gl);
    m_patchEbo.syncToGpu(gl);
    m_netEbo.syncToGpu(gl);

    if (!m_patchVao.created() && !m_patchEbo.empty()) {
        m_patchVao.setup(gl, patchSourceVbo(), m_patchEbo.vboId());
    }
    if (!m_netVao.created() && !m_netEbo.empty()) {
        m_netVao.setup(gl, m_registry->getPositionVBO(), m_netEbo.vboId());
    }
}
