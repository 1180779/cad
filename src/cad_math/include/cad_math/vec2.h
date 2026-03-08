//
// Created on 3/7/26.
//

#ifndef CAD_VEC2_H
#define CAD_VEC2_H

#include <array>

#include <cad_math/vec_base.h>
#include <cad_math/common.h>

namespace cadm
{
    template <>
    struct vec<2, cadf> : vec_base<vec<2, cadf>, 2, cadf>
    {
        union
        {
            struct
            {
                cadf x, y;
            };

            struct
            {
                cadf r, g;
            };

            std::array<cadf, 2> data;
        };

        vec() : x(0), y(0)
        {
        }

        vec(const cadf x, const cadf y) : x(x), y(y)
        {
        }

        constexpr static vec unitX() noexcept { return {1.0, 0.0}; }
        constexpr static vec unitY() noexcept { return {0.0, 1.0}; }
    };

    using vec2 = vec<2, cadf>;
}
#endif //CAD_VEC2_H
