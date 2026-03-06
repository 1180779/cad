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
    struct vec2 : vec_base<vec2, 2, cadf>
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

        vec2() : x(0), y(0)
        {
        }

        vec2(const cadf x, const cadf y) : x(x), y(y)
        {
        }

        constexpr static vec2 unitX() noexcept { return {1.0, 0.0}; }
        constexpr static vec2 unitY() noexcept { return {0.0, 1.0}; }
    };
}
#endif //CAD_VEC2_H
