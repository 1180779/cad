//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_VEC4_H
#define CAD_VEC4_H

#include <array>
#include <cad_math/vec_base.h>

namespace cadm
{
    struct vec4 : vec_base<vec4, 4, cadf>
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

        vec4() : x(0), y(0), z(0), w(0)
        {
        }

        vec4(const cadf x, const cadf y, const cadf z, const cadf w) : x(x), y(y), z(z), w(w)
        {
        }

        vec4 cross(const vec4& other) const
        {
            return vec4(x * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x, 0);
        }
    };
}

#endif //CAD_VEC4_H
