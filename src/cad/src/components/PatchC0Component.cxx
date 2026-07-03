//
// Created by Radosław Głasek on 02.07.2026
//

#include "PatchC0Component.hxx"

void PatchC0Component::rebuildPatchData() {
    const int patches = getPatchCount();
    std::vector<uint32_t> idx;
    idx.reserve(static_cast<size_t>(patches) * 16);

    std::array < int, 16 > grid{};
    for (int py = 0; py < m_patchCountY; ++py) {
        for (int px = 0; px < m_patchCountX; ++px) {
            gatherPatch(px, py, grid);
            for (const auto g : grid) {
                idx.push_back(m_controlPoints[g]);
            }
        }
    }
    m_patchEbo.diffAssign(std::move(idx));
}
