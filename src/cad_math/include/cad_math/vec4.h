//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_VEC4_H
#define CAD_VEC4_H

#include <array>

#include "vec_base.h"
#include "vec3.h"

namespace cadm
{
    template <>
    struct vec<4, cadf> : vec_base<vec<4, cadf>, 4, cadf>
    {
        union
        {
            struct
            {
                cadf x, y, z, w;
            };

            struct
            {
                cadf r, g, b, a;
            };

            std::array<cadf, 4> data;
        };

        vec() : x(0), y(0), z(0), w(0)
        {
        }

        vec(const cadf x, const cadf y, const cadf z, const cadf w) : x(x), y(y), z(z), w(w)
        {
        }

        vec(const vec3& v, const cadf w) : x(v.x), y(v.y), z(v.z), w(w)
        {
        }

        vec(const cadf x, const vec3& v) : x(x), y(v.x), z(v.y), w(v.z)
        {
        }

        vec cross(const vec& other) const
        {
            return {x * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x, 0};
        }

        constexpr static vec unitX() noexcept { return {1.0, 0.0, 0.0, 0.0}; }
        constexpr static vec unitY() noexcept { return {0.0, 1.0, 0.0, 0.0}; }
        constexpr static vec unitZ() noexcept { return {0.0, 0.0, 1.0, 0.0}; }
        constexpr static vec unitW() noexcept { return {0.0, 0.0, 0.0, 1.0}; }
    };

    using vec4 = vec<4, cadf>;
}

#endif //CAD_VEC4_H
