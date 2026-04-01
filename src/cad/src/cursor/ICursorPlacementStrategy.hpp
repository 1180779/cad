//
// Created on 3/31/26.
//

#ifndef CAD_ICURSORPLACEMENTSTRATEGY_HPP
#define CAD_ICURSORPLACEMENTSTRATEGY_HPP

#include <optional>
#include <cad_math/mat4.hpp>
#include <cad_math/vec3.hpp>

class ICursorPlacementStrategy
{
public:
    virtual ~ICursorPlacementStrategy() = default;

    virtual std::optional<cadm::vec3> resolve(
        QMouseEvent *event,
        int viewportW,
        int viewportH,
        const cadm::mat4 &invView,
        const cadm::mat4 &invProj) = 0;

    virtual void onGridPlanesChanged(int mask)
    {
    }
};

#endif //CAD_ICURSORPLACEMENTSTRATEGY_HPP
