//
// Created by Radosław Głasek on 23.06.2026
//

#ifndef CAD_INTERPC2SOLVER_HXX
#define CAD_INTERPC2SOLVER_HXX

#include <span>
#include <vector>

#include <cad_math/Vec3.hpp>

namespace interpC2 {
    /// @brief Fit a C2 cubic spline through @p points and emit its piecewise Bezier form
    ///
    /// @details Natural cubic spline with chord-length parametrization. Segment i is parametrized 
    /// on [0, |Q_{i+1}-Q_i|], and the interior second-derivative coefficients are found from 
    /// a tridiagonal system solved in O(n) by the Thomas algorithm. The resulting power-basis 
    /// cubics are converted to Bernstein control points so the existing cubic-patch tessellation 
    /// can render them unchanged.
    ///
    /// @note Shared-endpoint layout of 3 * segments + 1 positions (segment k occupies indices
    /// [3k, 3k+1, 3k+2, 3k+3]); the join Q_{k+1} is stored once
    ///
    /// @param points interpolation points in order
    /// @param out filled with 3 * segments + 1 Bernstein positions (empty if < 2 distinct points)
    void solve(std::span<const cadm::Vec3> points, std::vector<cadm::Vec3> &out);
}

#endif //CAD_INTERPC2SOLVER_HXX
