//
// Created on 4/18/26.
//

#ifndef CAD_BEZIERUTILS_HPP
#define CAD_BEZIERUTILS_HPP

#include <cad_math/Vec3.hpp>
#include <cad_math/Vec4.hpp>
#include <cad_math/Mat4.hpp>
#include <cad_math/Helpers.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <span>

#include "PointHandle.hpp"

namespace bezierUtils {
    /// @brief Cubic Bézier control points (also a patch row/column)
    using Curve4 = cadm::Vec<cadm::Vec3, 4>;

    /// @brief Handle counterpart of <tt>Curve4</tt>: the control points as
    /// registry handles
    using HandleCurve4 = cadm::Vec<PointHandle, 4>;

    /// @brief Four handle rows: a 4x4 control grid, or the four boundary edges
    /// of a single patch
    /// 
    /// @details for the 4x4 grid the points are laid out rows-wise (i.e., rows
    /// = const parameter u lines: [0] = const u(=0) line, [3] = const u(=3)
    /// line)
    using HandleGrid4x4 = cadm::Vec<HandleCurve4, 4>;

    /// @brief Position counterpart of <tt>HandleGrid4x4</tt>
    using Grid4x4 = cadm::Vec<Curve4, 4>;

    template <typename T>
    using generic4x4Grid = cadm::Vec<cadm::Vec<T, 4>, 4>;

    template <typename T>
    [[nodiscard]] generic4x4Grid<T> transposeGrid(const generic4x4Grid<T> &grid) {
        cadm::Mat<T, 4, 4> transposed{grid[0], grid[1], grid[2], grid[3]};
        return {transposed.row[0], transposed.row[1], transposed.row[2], transposed.row[3]};
    }

    inline cadm::Vec3 lerp(const cadm::Vec3 &a, const cadm::Vec3 &b, const cadm::cadf t) {
        return a + (b - a) * t;
    }

    /// @brief One in-place de Casteljau pass: reduces the first n points to n-1
    inline void deCasteljauStep(cadm::Vec3 *pts, const int n, const cadm::cadf t) {
        for (int i = 0; i + 1 < n; ++i) {
            pts[i] = lerp(pts[i], pts[i + 1], t);
        }
    }

    /// @brief Evaluate a cubic Bézier value and derivative at @p t (de
    /// Casteljau)
    /// @returns {value, derivative}
    [[nodiscard]] inline cadm::Vec<cadm::Vec3, 2> bezierValueAndDerivativeAt(Curve4 c, const cadm::cadf t) {
        deCasteljauStep(c.data.data(), 4, t);
        deCasteljauStep(c.data.data(), 3, t);
        const auto value = lerp(c[0], c[1], t);
        const auto derivative = cadm::cadf{3.0} * (-c[0] + c[1]);
        return {value, derivative};
    }

    /// @brief Evaluate a cubic Bézier derivative at @p t (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierDerivativeAt(Curve4 c, const cadm::cadf t) {
        deCasteljauStep(c.data.data(), 4, t);
        deCasteljauStep(c.data.data(), 3, t);
        return cadm::cadf{3.0} * (-c[0] + c[1]);
    }

    /// @brief Evaluate a cubic Bézier at @p t (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierAt(Curve4 c, const cadm::cadf t) {
        deCasteljauStep(c.data.data(), 4, t);
        deCasteljauStep(c.data.data(), 3, t);
        deCasteljauStep(c.data.data(), 2, t);
        return c[0];
    }

    /// @brief Evaluate a cubic Bézier patch at (@p u, @p v) (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierAt(const Grid4x4 &patch, const cadm::cadf u, const cadm::cadf v) {
        Curve4 row;
        for (int i = 0; i < 4; ++i) {
            row[i] = bezierAt(patch[i], u);
        }
        return bezierAt(row, v);
    }

    /// @brief Evaluate a cubic Bézier patch at @p uv (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierAt(const Grid4x4 &patch, const cadm::Vec2 uv) {
        return bezierAt(patch, uv.x, uv.y);
    }

    /// @brief Evaluate a cubic Bézier value and partial derivatives with
    /// respect to u and v at (@p u, @p v) (de Casteljau). 
    /// @returns {value, uDerivative, vDerivative}
    [[nodiscard]] inline cadm::Vec<cadm::Vec3, 3> bezierValueAndDerivativesAt(
        Grid4x4 patch,
        const cadm::cadf u,
        const cadm::cadf v
    ) {
        Curve4 uDerivativeCurve;
        for (int i = 0; i < 4; ++i) {
            deCasteljauStep(patch[i].data.data(), 4, u);
            deCasteljauStep(patch[i].data.data(), 3, u);
            uDerivativeCurve[i] = cadm::cadf{3.0} * (-patch[i][0] + patch[i][1]);
        }
        for (int i = 0; i < 4; ++i) {
            deCasteljauStep(patch[i].data.data(), 2, u);
        }
        Curve4 valueCurve{patch[0][0], patch[1][0], patch[2][0], patch[3][0]};
        deCasteljauStep(valueCurve.data.data(), 4, v);
        deCasteljauStep(valueCurve.data.data(), 3, v);

        const auto uDerivative = bezierAt(uDerivativeCurve, v);
        const auto vDerivative = 3 * (-valueCurve[0] + valueCurve[1]);
        deCasteljauStep(valueCurve.data.data(), 2, v);
        const auto value = valueCurve[0];
        return {value, uDerivative, vDerivative};
    }

    /// @brief Evaluate a cubic Bézier derivative with respect to u at (@p u, @p
    /// v) (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierUDerivativeAt(const Grid4x4 &patch, const cadm::cadf u, const cadm::cadf v) {
        Curve4 row;
        for (int i = 0; i < 4; ++i) {
            row[i] = bezierDerivativeAt(patch[i], u);
        }
        return bezierAt(row, v);
    }

    /// @brief Evaluate a cubic Bézier derivative with respect to u at (@p u, @p
    /// v) (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierUDerivativeAt(const Grid4x4 &patch, const cadm::Vec2 uv) {
        return bezierUDerivativeAt(patch, uv.x, uv.y);
    }

    /// @brief Evaluate a cubic Bézier derivative with respect to v at (@p u, @p
    /// v) (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierVDerivativeAt(const Grid4x4 &patch, const cadm::cadf u, const cadm::cadf v) {
        Curve4 row;
        for (int i = 0; i < 4; ++i) {
            row[i] = bezierAt(patch[i], u);
        }
        return bezierDerivativeAt(row, v);
    }

    /// @brief Evaluate a cubic Bézier derivative with respect to v at (@p u, @p
    /// v) (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierVDerivativeAt(const Grid4x4 &patch, const cadm::Vec2 uv) {
        return bezierVDerivativeAt(patch, uv.x, uv.y);
    }

    /// @brief Evaluate a cubic Bézier derivative with respect to u or v at (@p
    /// u, @p v) (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierDerivativeAt(const Grid4x4 &patch, const cadm::Vec2 uv, const int i) {
        assert(i == 0 || i == 1);
        switch (i) {
        case 0:
            return bezierUDerivativeAt(patch, uv);
        case 1:
            return bezierVDerivativeAt(patch, uv);
        default:
            break;
        }
        return {};
    }

    /// @brief Split a cubic Bézier at t = 1/2
    /// @returns The two halves
    [[nodiscard]] inline std::array<Curve4, 2> splitInHalf(const Curve4 &c) {
        const cadm::Vec3 q0 = lerp(c[0], c[1], 0.5f);
        const cadm::Vec3 q1 = lerp(c[1], c[2], 0.5f);
        const cadm::Vec3 q2 = lerp(c[2], c[3], 0.5f);
        const cadm::Vec3 r0 = lerp(q0, q1, 0.5f);
        const cadm::Vec3 r1 = lerp(q1, q2, 0.5f);
        const cadm::Vec3 m = lerp(r0, r1, 0.5f);
        return {
            Curve4{c[0], q0, r0, m},
            Curve4{m, r1, q2, c[3]},
        };
    }

    /// Compute the screen-space bounding box extent (in pixels) of control
    /// points
    /// @returns Optional int value representing the bigger of width and height
    /// of the polygon span by the points or none if the polygon is not within
    /// the screen
    [[nodiscard]] inline std::optional<int> screenExtent(
        const std::span<const cadm::Vec3> pts,
        const cadm::Mat4 &view,
        const cadm::Mat4 &proj,
        const int vpW,
        const int vpH
    ) {
        int minX = vpW, maxX = 0, minY = vpH, maxY = 0;
        bool anyVisible = false;
        for (const auto &pt : pts) {
            const auto sp = cadm::projectToScreenGl(pt, view, proj, vpW, vpH);
            if (!sp) {
                continue;
            }
            anyVisible = true;
            minX = std::min(minX, sp->x);
            maxX = std::max(maxX, sp->x);
            minY = std::min(minY, sp->y);
            maxY = std::max(maxY, sp->y);
        }
        if (!anyVisible) {
            return std::nullopt;
        }
        return std::max(maxX - minX, maxY - minY);
    }
}

#endif //CAD_BEZIERUTILS_HPP
