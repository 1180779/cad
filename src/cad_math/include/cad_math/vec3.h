//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_VEC3_H
#define CAD_VEC3_H

#include <array>

#include "vec_base.h"

namespace cadm
{
    template <>
    struct vec<3, cadf> : vec_base<vec<3, cadf>, 3, cadf>
    {
        union
        {
            struct
            {
                cadf x, y, z;
            };

            struct
            {
                cadf r, g, b;
            };

            std::array<cadf, 3> data;
        };

        constexpr vec() : x(0), y(0), z(0)
        {
        }

        constexpr vec(const cadf x, const cadf y, const cadf z) : x(x), y(y), z(z)
        {
        }

        constexpr static vec unitX() noexcept { return {1.0, 0.0, 0.0}; }
        constexpr static vec unitY() noexcept { return {0.0, 1.0, 0.0}; }
        constexpr static vec unitZ() noexcept { return {0.0, 0.0, 1.0}; }

        [[nodiscard]] constexpr vec cross(const vec& other) const
        {
            return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
        }
    };

    using vec3 = vec<3, cadf>;
}

#endif //CAD_VEC3_H
