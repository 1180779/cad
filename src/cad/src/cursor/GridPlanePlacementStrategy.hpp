//
// Created on 3/31/26.
//

#ifndef CAD_GRIDPLANEPLACEMENTSTRATEGY_HPP
#define CAD_GRIDPLANEPLACEMENTSTRATEGY_HPP

#include <QMouseEvent>

#include "ICursorPlacementStrategy.hpp"

/// @brief Resolves cursor placement by intersecting the mouse ray with the most
/// view-facing active grid plane.
///
/// When multiple planes are active, the one whose normal is most aligned
/// with the camera's view direction is chosen.
///
/// Returns nullopt when the ray is nearly parallel to all active planes
class GridPlanePlacementStrategy final : public IViewportPositionStrategy {
public:
    explicit GridPlanePlacementStrategy(const int gridPlanesMask) : m_gridPlanesMask(gridPlanesMask) {}

    void onGridPlanesChanged(const int mask) override {
        m_gridPlanesMask = mask;
    }

    std::optional<cadm::vec3> resolve(
        QMouseEvent *event,
        int viewportW,
        int viewportH,
        const cadm::mat4 &invView,
        const cadm::mat4 &invProj
    ) override;

private:
    int m_gridPlanesMask;
    static constexpr cadm::cadf s_parallelThreshold = cadm::feps;
};

#endif //CAD_GRIDPLANEPLACEMENTSTRATEGY_HPP
