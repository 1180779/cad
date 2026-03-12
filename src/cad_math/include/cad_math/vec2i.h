//
// Created on 3/7/26.
//

#ifndef CAD_VEC2I_H
#define CAD_VEC2I_H

#include <array>

#include "vec_base.h"

namespace cadm
{
    template <>
    struct vec<2, int> : vec_base<vec<2, int>, 2, int>
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

        constexpr vec() : x(0), y(0)
        {
        }

        constexpr vec(const int x, const int y) : x(x), y(y)
        {
        }

        constexpr static vec unitX() noexcept { return {1, 0}; }
        constexpr static vec unitY() noexcept { return {0, 1}; }
    };

    using vec2i = vec<2, int>;
}

#endif //CAD_VEC2I_H
