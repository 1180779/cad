//
// Created by Radosław Głasek on 03.07.2026
//

#include "PatchC2Component.hxx"

#include <array>
#include <numeric>
#include <span>

#include "../../utils/BSplineToBezierConverter.hpp"
#include "GlCommon.hpp"

PatchC2Component::~PatchC2Component() {
    m_bernsteinVbo.deleteGpu(getGl());
}

void PatchC2Component::regenerateMesh() {
    std::vector<cadm::Vec3> verts(static_cast<size_t>(getPatchCount()) * 16);

    std::array<int, 16> grid{};
    size_t base = 0;
    for (int py = 0; py < m_patchCountY; ++py) {
        for (int px = 0; px < m_patchCountX; ++px) {
            gatherPatch(px, py, grid);

            std::array<cadm::Vec3, 16> tmp{};
            for (int i = 0; i < 4; ++i) {
                bsplineToBezier::uniformSegment(
                    m_registry->getPosition(m_controlPoints[grid[i * 4 + 0]]),
                    m_registry->getPosition(m_controlPoints[grid[i * 4 + 1]]),
                    m_registry->getPosition(m_controlPoints[grid[i * 4 + 2]]),
                    m_registry->getPosition(m_controlPoints[grid[i * 4 + 3]]),
                    std::span<cadm::Vec3, 4>(std::span{tmp}.subspan(static_cast<size_t>(i) * 4, 4))
                );
            }
            for (int j = 0; j < 4; ++j) {
                std::array<cadm::Vec3, 4> col{};
                bsplineToBezier::uniformSegment(
                    tmp[0 * 4 + j],
                    tmp[1 * 4 + j],
                    tmp[2 * 4 + j],
                    tmp[3 * 4 + j],
                    std::span(col)
                );
                for (int i = 0; i < 4; ++i) {
                    verts[base + static_cast<size_t>(i) * 4 + j] = col[i];
                }
            }
            base += 16;
        }
    }
    m_bernsteinVbo.diffAssign(std::move(verts));
}

void PatchC2Component::rebuildPatchData() {
    std::vector<uint32_t> idx(static_cast<size_t>(getPatchCount()) * 16);
    std::ranges::iota(idx, 0u);
    m_patchEbo.diffAssign(std::move(idx));
}
