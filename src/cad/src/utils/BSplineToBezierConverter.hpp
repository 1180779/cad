//
// Created on 5/23/26.
//

#ifndef CAD_BSPLINETOBEZIERCONVERTER_HPP
#define CAD_BSPLINETOBEZIERCONVERTER_HPP

#include <span>
#include <vector>

#include <cad_math/Vec3.hpp>
#include "../PointRegistry.hpp"

namespace bsplineToBezier {
    /// @brief Convert one uniform cubic B-spline segment to Bernstein control points
    /// @param d0, d1, d2, d3  four consecutive de Boor points for this segment
    /// @param view span into which the four Bezier control points (b0,b1,b2,b3) will be saved
    void uniformSegment(
        cadm::Vec3 d0,
        cadm::Vec3 d1,
        cadm::Vec3 d2,
        cadm::Vec3 d3,
        std::span<cadm::Vec3, 4> view
    );

    /// @brief Convert all segments of an open cubic B-spline to piecewise Bezier with uniform
    /// parametrization
    ///
    /// @note Uses a shared-endpoint layout of 3 * segments + 1 positions: the join
    /// point shared by adjacent segments is stored once (segment k's b3 is the same
    /// slot as segment k+1's b0). Segment k occupies indices [3k, 3k+1, 3k+2, 3k+3]
    ///
    /// @param handles de Boor point handles in order (n >= 4 required)
    /// @param registry position source
    /// @param out filled with 3 * segments + 1 Bernstein positions (empty if n < 4)
    void convert(
        std::span<const PointHandle> handles,
        const PointRegistry &registry,
        std::vector<cadm::Vec3> &out
    );
}

#endif //CAD_BSPLINETOBEZIERCONVERTER_HPP
