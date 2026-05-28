//
// Created on 3/31/26.
//

#ifndef CAD_ICURSORPLACEMENTSTRATEGY_HPP
#define CAD_ICURSORPLACEMENTSTRATEGY_HPP

#include <optional>
#include <cad_math/mat4.hpp>
#include <cad_math/vec3.hpp>

/// Generic strategy for resolving a mouse event to a 3D world-space position.
///
/// Used for cursor placement, control-point dragging, and click-to-add modes.
class IViewportPositionStrategy {
public:
    virtual ~IViewportPositionStrategy() = default;

    virtual std::optional<cadm::vec3> resolve(
        QMouseEvent *event,
        int viewportW,
        int viewportH,
        const cadm::mat4 &invView,
        const cadm::mat4 &invProj
    ) = 0;

    virtual void onGridPlanesChanged(int mask) {}
};

#endif //CAD_ICURSORPLACEMENTSTRATEGY_HPP
