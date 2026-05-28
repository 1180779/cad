//
// Created on 5/23/26.
//

#ifndef CAD_BSPLINETOBEZIERCONVERTER_HPP
#define CAD_BSPLINETOBEZIERCONVERTER_HPP

#include <span>
#include <vector>

#include <cad_math/vec3.hpp>
#include "PointRegistry.hpp"

/// @brief how parameter values are assigned to knot intervals between de Boor points
enum class ParametrizationMode {
    /// @brief uniform parametrization
    ///
    /// @details
    /// Each knot interval has width 1.
    /// B-spline to Bernstein conversion:
    ///
    ///   [b0]   1 [1 4 1 0] [d_i  ]
    ///   [b1] = - [0 4 2 0] [d_i+1]
    ///   [b2]   6 [0 2 4 0] [d_i+2]
    ///   [b3]     [0 1 4 1] [d_i+3]
    ///
    /// Prone to loops/cusps when de Boor points are unevenly spaced
    uniform,

    /// @brief parametrization from chord length
    /// 
    /// @details 
    /// Knot interval proportional to ||d_{i + 1} - d_i|| (chord length).
    /// Converted to Bezier via Boehm knot insertion.
    /// Suppresses loops caused by uneven point spacing
    chordLength,
};

namespace bsplineToBezier {
    /// @brief convert one B-spline segment (uniform knots) to Bernstein control points.
    /// @param d0, d1, d2, d3  four consecutive de Boor points for this segment
    /// @param view span into which the four Bezier control points (b0,b1,b2,b3) will be saved
    void uniformSegment(
        cadm::vec3 d0,
        cadm::vec3 d1,
        cadm::vec3 d2,
        cadm::vec3 d3,
        std::span<cadm::vec3, 4> &view
    );

    namespace detail {
        /// @brief insert knot tHat once into a degree-p B-spline (Boehm's algorithm).
        /// Span k is the largest i such that knots[i] <= tHat < knots[i+1]
        void insertKnot(
            std::vector<cadm::vec3> &pts,
            std::vector<float> &knots,
            float tHat,
            int p
        );
    }

    /// @brief convert all segments of an open cubic B-spline to piecewise Bezier using
    /// chord-length knot intervals and Boehm knot insertion
    ///
    /// @param handles de Boor point handles in order (n >= 4 required)
    /// @param registry position source
    /// @param out filled with segments * 4 Bernstein positions
    void chordLength(
        std::span<const PointHandle> handles,
        const PointRegistry &registry,
        std::vector<cadm::vec3> &out
    );

    /// @brief convert all segments of an open cubic B-spline to piecewise Bezier with uniform parametrization
    ///
    /// @param handles de Boor point handles in order (n >= 4 required)
    /// @param registry position source
    /// @param out filled with segments * 4 Bernstein positions
    void uniform(
        std::span<const PointHandle> handles,
        const PointRegistry &registry,
        std::vector<cadm::vec3> &out
    );

    /// @brief dispatch to the appropriate converter
    void convert(
        ParametrizationMode mode,
        std::span<const PointHandle> handles,
        const PointRegistry &registry,
        std::vector<cadm::vec3> &out
    );
}

#endif //CAD_BSPLINETOBEZIERCONVERTER_HPP
