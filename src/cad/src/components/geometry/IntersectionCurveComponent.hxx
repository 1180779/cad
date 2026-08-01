//
// Created by Radosław Głasek on 01.08.2026
//

#ifndef CAD_INTERSECTIONCURVECOMPONENT_HXX
#define CAD_INTERSECTIONCURVECOMPONENT_HXX

#include <vector>

#include "../GeometryComponent.hpp"
#include <cad_math/Vec2.hpp>

/// @brief Curve traced along the intersection of two surfaces
///
/// @note The curve is computed once at creation from the traced points; it does
/// not react to later edits of the source surfaces
class IntersectionCurveComponent final : public GeometryComponent {
public:
    IntersectionCurveComponent(
        EntityId patch1,
        EntityId patch2,
        std::vector<cadm::Vec3> points3D,
        std::vector<cadm::Vec2> params1,
        std::vector<cadm::Vec2> params2,
        bool closed
    );

    [[nodiscard]] EntityId getPatch1() const {
        return m_patch1;
    }

    [[nodiscard]] EntityId getPatch2() const {
        return m_patch2;
    }

    [[nodiscard]] bool isClosed() const {
        return m_closed;
    }

    /// @brief Curve points in patch1's (u, v) parameter space
    [[nodiscard]] const std::vector<cadm::Vec2>& getParams1() const {
        return m_params1;
    }

    /// @brief Curve points in patch2's (u, v) parameter space
    [[nodiscard]] const std::vector<cadm::Vec2>& getParams2() const {
        return m_params2;
    }

    /// @brief Traced 3D points, one per curve sample
    [[nodiscard]] const std::vector<cadm::Vec3>& getPoints3D() const {
        return m_points3D;
    }

    void regenerateMesh() override;

private:
    EntityId m_patch1;
    EntityId m_patch2;
    std::vector<cadm::Vec3> m_points3D;
    std::vector<cadm::Vec2> m_params1;
    std::vector<cadm::Vec2> m_params2;
    bool m_closed;
};

#endif //CAD_INTERSECTIONCURVECOMPONENT_HXX
