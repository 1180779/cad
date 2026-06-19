//
// Created on 3/31/26.
//

#ifndef CAD_ICURSORPLACEMENTSTRATEGY_HPP
#define CAD_ICURSORPLACEMENTSTRATEGY_HPP

#include <QMouseEvent>

#include <optional>
#include <cad_math/Mat4.hpp>
#include <cad_math/Vec3.hpp>

/// @brief Generic strategy for resolving a mouse event to a 3D world-space position.
///
/// Used for cursor placement, control-point dragging, and click-to-add modes
class IViewportPositionStrategy {
public:
    virtual ~IViewportPositionStrategy() = default;

    virtual std::optional<cadm::Vec3> resolve(
        QMouseEvent *event,
        int viewportW,
        int viewportH,
        const cadm::Mat4 &invView,
        const cadm::Mat4 &invProj
    ) = 0;

    virtual void onGridPlanesChanged(int mask) {}
};

#endif //CAD_ICURSORPLACEMENTSTRATEGY_HPP
