//
// Created on 4/18/26.
//

#ifndef CAD_BEZIERUTILS_HPP
#define CAD_BEZIERUTILS_HPP

#include <cad_math/vec3.hpp>
#include <cad_math/mat4.hpp>
#include <cad_math/helpers.hpp>
#include <algorithm>
#include <cmath>

namespace bezierUtils {
    /// Compute the screen-space bounding box extent (in pixels) of 4 control points
    /// @returns Optional int value representing the bigger of width and height
    /// of the polygon span by the points or none if the polygon is not within the screen
    inline std::optional<int> screenExtent(
        const cadm::vec3 pts[4],
        const cadm::mat4 &view,
        const cadm::mat4 &proj,
        const int vpW,
        const int vpH
    ) {
        int minX = vpW, maxX = 0, minY = vpH, maxY = 0;
        bool anyVisible = false;
        for (int i = 0; i < 4; ++i) {
            const auto sp = cadm::projectToScreenGL(pts[i], view, proj, vpW, vpH);
            if (!sp) { continue; }
            anyVisible = true;
            minX = std::min(minX, sp->x);
            maxX = std::max(maxX, sp->x);
            minY = std::min(minY, sp->y);
            maxY = std::max(maxY, sp->y);
        }
        if (!anyVisible) { return std::nullopt; }
        return std::max(maxX - minX, maxY - minY);
    }
}

#endif //CAD_BEZIERUTILS_HPP
