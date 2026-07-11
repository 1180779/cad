//
// Created by Radosław Głasek on 02.07.2026
//

#include "PatchC0Component.hxx"

::std::optional<bezierUtils::Grid4x4> PatchC0Component::patchAtUv(const cadm::cadf u, const cadm::cadf v) const {
    const auto loc = resolveUv(u, v);
    if (!loc) {
        return std::nullopt;
    }
    std::array<int, 16> patch;
    std::array<cadm::Vec3, 16> patchPos;
    gatherPatch(loc->xPatch, loc->yPatch, patch);
    for (int i = 0; i < 16; ++i) {
        patchPos[i] = m_registry->getPosition(patch[i]);
    }
    return bezierUtils::grid4x4(patchPos);
}

void PatchC0Component::rebuildPatchData() {
    const int patches = getPatchCount();
    std::vector<uint32_t> idx;
    idx.reserve(static_cast<size_t>(patches) * 16);

    std::array<int, 16> grid{};
    for (auto [py, px] : patchCoords()) {
        gatherPatch(px, py, grid);
        for (const auto g : grid) {
            idx.push_back(m_controlPoints[g]);
        }
    }
    m_patchEbo.diffAssign(std::move(idx));
}
