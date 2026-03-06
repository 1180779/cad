//
// Created on 3/3/26.
//

#ifndef CAD_VEC3I_H
#define CAD_VEC3I_H

#include <array>

#include "vec_base.h"

namespace cadm
{
    struct vec3i : cadm::vec_base<vec3i, 3, int>
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

        vec3i() : x(0), y(0), z(0)
        {
        }

        vec3i(const int x, const int y, const int z) : x(x), y(y), z(z)
        {
        }

        constexpr static vec3i unitX() noexcept { return {1, 0, 0}; }
        constexpr static vec3i unitY() noexcept { return {0, 1, 0}; }
        constexpr static vec3i unitZ() noexcept { return {0, 0, 1}; }
    };
}

#endif //CAD_VEC3I_H
