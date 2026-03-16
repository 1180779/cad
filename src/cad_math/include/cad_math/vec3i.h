//
// Created on 3/3/26.
//

#ifndef CAD_VEC3I_H
#define CAD_VEC3I_H

#include <array>

#include "vec_base.h"

namespace cadm
{
    template <>
    struct vec<3, int> : cadm::vec_base<vec<3, int>, 3, int>
    {
        union
        {
            struct
            {
                int x, y, z;
            };

            struct
            {
                int r, g, b;
            };

            std::array<int, 3> data;
        };

        constexpr vec()
            : x(0), y(0), z(0)
        {
        }

        constexpr vec(const int x, const int y, const int z)
            : x(x), y(y), z(z)
        {
        }

        constexpr static vec unitX() noexcept { return {1, 0, 0}; }
        constexpr static vec unitY() noexcept { return {0, 1, 0}; }
        constexpr static vec unitZ() noexcept { return {0, 0, 1}; }
    };

    using vec3i = vec<3, int>;
}

#endif //CAD_VEC3I_H