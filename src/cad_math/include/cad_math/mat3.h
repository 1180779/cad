//
// Created on 3/7/26.
//

#ifndef CAD_MAT3_H
#define CAD_MAT3_H
#include "common.h"
#include "mat_base.h"
#include "vec2.h"
#include "vec3.h"

namespace cadm
{
    template <std::size_t R, std::size_t C, typename T>
    struct mat;

    template <>
    struct mat<3, 3, cadf> : public mat_base<mat, mat<3, 3, cadf>, vec3, vec3, 3, 3, cadf>
    {
        union
        {
            cadf data[9]{};
            vec3 columns[3];
        };

        constexpr mat()
        {
            for (auto& cell : data)
                cell = 0;
        }

        constexpr mat(const cadf x0, const cadf x1, const cadf x2,
            const cadf y0, const cadf y1, const cadf y2,
            const cadf z0, const cadf z1, const cadf z2)
        {
            columns[0] = vec3(x0, x1, x2);
            columns[1] = vec3(y0, y1, y2);
            columns[2] = vec3(z0, z1, z2);
        }

        constexpr mat(const vec3& c0, const vec3& c1, const vec3& c2)
        {
            columns[0] = c0;
            columns[1] = c1;
            columns[2] = c2;
        }

        constexpr static mat identity()
        {
            return {
                1, 0, 0,
                0, 1, 0,
                0, 0, 1,
            };
        }

        constexpr static mat scale(const vec2& s) { return scale(s.x, s.y); }

        constexpr static mat scale(const cadf sx, const cadf sy)
        {
            return diag(sx, sy, 1.0);
        }

        constexpr static mat diag(const cadf m0, const cadf m1, const cadf m2)
        {
            return {
                m0, 0, 0,
                0, m1, 0,
                0, 0, m2,
            };
        }

        constexpr static mat translation(const vec2& t) { return translation(t.x, t.y); }

        constexpr static mat translation(const cadf tx, const cadf ty)
        {
            return {
                1, 0, 0,
                0, 1, 0,
                tx, ty, 1
            };
        }

        static mat rotX(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                1, 0, 0,
                0, c, s,
                0, -s, c,
            };
        }

        static mat rotY(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                c, 0, -s,
                0, 1, 0,
                s, 0, c,
            };
        }
    };

    using mat3 = mat<3, 3, cadf>;
}
#endif //CAD_MAT3_H
