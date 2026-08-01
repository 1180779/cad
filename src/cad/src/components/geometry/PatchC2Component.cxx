//
// Created by Radosław Głasek on 03.07.2026
//

#include "PatchC2Component.hxx"

#include <algorithm>
#include <array>
#include <span>

#include "../../utils/BSplineToBezierConverter.hpp"
#include "GlCommon.hpp"

PatchC2Component::~PatchC2Component() {
    m_bernsteinVbo.deleteGpu(getGl());
}

std::array<cadm::Vec3, 16> PatchC2Component::bernsteinNet(const int px, const int py) const {
    std::array<int, 16> grid{};
    gatherPatch(px, py, grid);

    // convert along v (within each row), then along u (down each column)
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

    std::array<cadm::Vec3, 16> net{};
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
            net[static_cast<size_t>(i) * 4 + j] = col[i];
        }
    }
    return net;
}

void PatchC2Component::regenerateMesh() {
    std::vector<cadm::Vec3> verts(static_cast<size_t>(getPatchCount()) * 16);

    size_t base = 0;
    for (int py = 0; py < m_patchCountY; ++py) {
        for (int px = 0; px < m_patchCountX; ++px) {
            const auto net = bernsteinNet(px, py);
            std::ranges::copy(net, verts.begin() + static_cast<std::ptrdiff_t>(base));
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

std::optional<bezierUtils::Grid4x4> PatchC2Component::patchAtUv(const cadm::cadf u, const cadm::cadf v) const {
    const auto loc = resolveUv(u, v);
    if (!loc) {
        return std::nullopt;
    }
    return bezierUtils::grid4x4(bernsteinNet(loc->xPatch, loc->yPatch));
}
