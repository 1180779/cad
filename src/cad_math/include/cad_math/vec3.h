//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_VEC3_H
#define CAD_VEC3_H

#include <array>
#include <cad_math/vec_base.h>

namespace cadm
{
    struct vec3 : vec_base<vec3, 3, cadf>
    {
        union
        {
            struct
            {
                cadf x, y, z;
            };

            std::array<cadf, 3> data;
        };

        vec3() : x(0), y(0), z(0)
        {
        }

        vec3(const cadf x, const cadf y, const cadf z) : x(x), y(y), z(z)
        {
        }

        [[nodiscard]] vec3 cross(const vec3& other) const
        {
            return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
        }
    };
}

#endif //CAD_VEC3_H
