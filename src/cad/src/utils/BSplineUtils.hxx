//
// Created by Radosław Głasek on 02.07.2026
//

#ifndef CAD_BSPLINEUTILS_HXX
#define CAD_BSPLINEUTILS_HXX

#include <span>

#include <cad_math/Vec3.hpp>

/// @brief Pure uniform cubic B-spline to Bézier conversions
namespace bsplineUtils {
    /// @brief Convert one uniform cubic B-spline segment (4 de Boor points) to
    /// its 4 Bernstein control points
    inline void uniformSegment(
        const cadm::Vec3 d0,
        const cadm::Vec3 d1,
        const cadm::Vec3 d2,
        const cadm::Vec3 d3,
        std::span<cadm::Vec3, 4> view
    ) {
        const auto prevLegTwoThird = d0 * (1.0f / 3.0f) + d1 * (2.0f / 3.0f);
        const auto legOneThird = d1 * (2.0f / 3.0f) + d2 * (1.0f / 3.0f);
        const auto legTwoThird = d1 * (1.0f / 3.0f) + d2 * (2.0f / 3.0f);
        const auto nextLegOneThird = d2 * (2.0f / 3.0f) + d3 * (1.0f / 3.0f);

        view[0] = prevLegTwoThird * 0.5f + legOneThird * 0.5f;
        view[1] = legOneThird;
        view[2] = legTwoThird;
        view[3] = legTwoThird * 0.5f + nextLegOneThird * 0.5f;
    }

    /// @brief Convert one bicubic uniform B-spline patch (4x4 de Boor net) to
    /// its 4x4 Bernstein net by applying the 1D uniformSegment conversion
    /// twice: once along each row, then once along each column of the result
    /// @param deBoor 16 de Boor points, laid out k = i*4 + j
    /// @param out 16 Bernstein points, same layout
    inline void uniformPatch(
        const std::span<const cadm::Vec3, 16> deBoor,
        const std::span<cadm::Vec3, 16> out
    ) {
        cadm::Vec3 tmp[16];
        for (int i = 0; i < 4; ++i) {
            uniformSegment(
                deBoor[i * 4 + 0],
                deBoor[i * 4 + 1],
                deBoor[i * 4 + 2],
                deBoor[i * 4 + 3],
                std::span<cadm::Vec3, 4>(tmp + i * 4, 4)
            );
        }
        for (int j = 0; j < 4; ++j) {
            cadm::Vec3 col[4];
            uniformSegment(
                tmp[0 * 4 + j],
                tmp[1 * 4 + j],
                tmp[2 * 4 + j],
                tmp[3 * 4 + j],
                std::span(col)
            );
            for (int i = 0; i < 4; ++i) {
                out[i * 4 + j] = col[i];
            }
        }
    }
}

#endif //CAD_BSPLINEUTILS_HXX
