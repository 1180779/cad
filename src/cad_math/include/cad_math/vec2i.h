//
// Created on 3/7/26.
//

#ifndef CAD_VEC2I_H
#define CAD_VEC2I_H

#include <array>

#include "vec_base.h"

namespace cadm
{
    struct vec2i : vec_base<vec2i, 2, int>
    {
        union
        {
            struct
            {
                int x, y;
            };

            struct
            {
                int r, g;
            };

            std::array<int, 2> data;
        };

        vec2i() : x(0), y(0)
        {
        }

        vec2i(const int x, const int y) : x(x), y(y)
        {
        }

        constexpr static vec2i unitX() noexcept { return {1, 0}; }
        constexpr static vec2i unitY() noexcept { return {0, 1}; }
    };
}

#endif //CAD_VEC2I_H