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
    using HandleGrid4x4 = cadm::Vec<HandleCurve4, 4>;

    /// @brief Position counterpart of <tt>HandleGrid4x4</tt>
    using Grid4x4 = cadm::Vec<Curve4, 4>;

    inline cadm::Vec3 lerp(const cadm::Vec3 &a, const cadm::Vec3 &b, const cadm::cadf t) {
        return a + (b - a) * t;
    }

    /// @brief Evaluate a cubic Bézier at @p t (de Casteljau)
    [[nodiscard]] inline cadm::Vec3 bezierAt(const Curve4 &c, const cadm::cadf t) {
        const cadm::Vec3 q0 = lerp(c[0], c[1], t);
        const cadm::Vec3 q1 = lerp(c[1], c[2], t);
        const cadm::Vec3 q2 = lerp(c[2], c[3], t);
        const cadm::Vec3 r0 = lerp(q0, q1, t);
        const cadm::Vec3 r1 = lerp(q1, q2, t);
        return lerp(r0, r1, t);
    }

    /// @brief Split a cubic Bézier at t = 1/2
    /// @return The two halves
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
    inline std::optional<int> screenExtent(
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
