//
// Created by Radosław Głasek on 01.08.2026
//

#include "IntersectionCurveComponent.hxx"

IntersectionCurveComponent::IntersectionCurveComponent(
    const EntityId patch1,
    const EntityId patch2,
    std::vector<cadm::Vec3> points3D,
    std::vector<cadm::Vec2> params1,
    std::vector<cadm::Vec2> params2,
    const bool closed
)
: m_patch1(patch1),
  m_patch2(patch2),
  m_points3D(std::move(points3D)),
  m_params1(std::move(params1)),
  m_params2(std::move(params2)),
  m_closed(closed) {
    regenerateMesh();
}

void IntersectionCurveComponent::regenerateMesh() {
    m_vertices.clear();
    m_lineIndices.clear();

    for (const auto &p : m_points3D) {
        m_vertices.push_back({p, {}, {1, 1, 0, 1}});
    }

    const auto n = static_cast<uint32_t>(m_points3D.size());
    for (uint32_t i = 0; i + 1 < n; ++i) {
        m_lineIndices.push_back(i);
        m_lineIndices.push_back(i + 1);
    }
    if (m_closed && n > 2) {
        m_lineIndices.push_back(n - 1);
        m_lineIndices.push_back(0);
    }
}
