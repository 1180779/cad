//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MAT4_H
#define CAD_MAT4_H

#include "mat_base.h"
#include "vec4.h"

#include "mat3.h"

namespace cadm
{
    template <std::size_t R, std::size_t C, typename T>
    struct mat;

    template <>
    struct mat<4, 4, cadf> : public mat_base<mat, mat<4, 4, cadf>, vec4, vec4, 4, 4, cadf>
    {
        union
        {
            cadf data[16]{};
            vec4 columns[4];
        };

        constexpr mat()
        {
            for (auto& cell : data)
                cell = 0;
        }

        constexpr mat(const cadf x0, const cadf x1, const cadf x2, const cadf x3,
            const cadf y0, const cadf y1, const cadf y2, const cadf y3,
            const cadf z0, const cadf z1, const cadf z2, const cadf z3,
            const cadf w0, const cadf w1, const cadf w2, const cadf w3)
        {
            columns[0] = vec4(x0, x1, x2, x3);
            columns[1] = vec4(y0, y1, y2, y3);
            columns[2] = vec4(z0, z1, z2, z3);
            columns[3] = vec4(w0, w1, w2, w3);
        }

        constexpr mat(const vec4& c0, const vec4& c1, const vec4& c2, const vec4& c3)
        {
            columns[0] = c0;
            columns[1] = c1;
            columns[2] = c2;
            columns[3] = c3;
        }

        constexpr static mat identity()
        {
            return mat{
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        }

        // useful article to revisit view matrix:
        // https://www.3dgep.com/understanding-the-view-matrix/

        // Look at matrix for Right Handed coordinate system
        constexpr static mat lookAtRH(const vec3& eye, const vec3& target, const vec3& up)
        {
            const vec3 zAxis = (eye - target).normalized(); // forward
            const vec3 xAxis = up.cross(zAxis).normalized(); // right
            const vec3 yAxis = zAxis.cross(xAxis); // up

            return {
                vec4(xAxis.x, yAxis.x, zAxis.x, 0.0),
                vec4(xAxis.y, yAxis.y, zAxis.y, 0.0),
                vec4(xAxis.z, yAxis.z, zAxis.z, 0.0),
                vec4(-xAxis.dot(eye), -yAxis.dot(eye), -zAxis.dot(eye), 1.0),
            };
        }

        // static mat perspective(const cadf fov, const cadf aspect, const cadf near, const cadf far)
        // {
        // }

        constexpr static mat ortho(const cadf left, const cadf right, const cadf bottom, const cadf top, const cadf near,
                         const cadf far)
        {
            return {
                vec4(2.0 / (right - left), 0.0, 0.0, 0.0),
                vec4(0.0, 2.0 / (top - bottom), 0.0, 0.0),
                vec4(0.0, 0.0, -2.0 / (far - near), 0.0),
                vec4(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near),
                     1.0)
            };
        }

        constexpr static mat scale(const vec3& s) { return scale(s.x, s.y, s.z); }

        constexpr static mat scale(const cadf sx, const cadf sy, const cadf sz)
        {
            return diag(sx, sy, sz, 1.0);
        }

        constexpr static mat diag(const cadf d0, const cadf d1, const cadf d2, const cadf d3)
        {
            return mat{
                d0, 0, 0, 0,
                0, d1, 0, 0,
                0, 0, d2, 0,
                0, 0, 0, d3
            };
        }

        constexpr static mat translation(const vec3& t) { return translation(t.x, t.y, t.z); }

        constexpr static mat translation(const cadf tx, const cadf ty, const cadf tz)
        {
            return {
                vec4::unitX(),
                vec4::unitY(),
                vec4::unitZ(),
                vec4(tx, ty, tz, 1.0),
            };
        }

        static mat rotX(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                vec4::unitX(),
                vec4(0, c, s, 0),
                vec4(0, -s, c, 0),
                vec4::unitW()
            };
        }

        static mat rotY(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                vec4(c, 0, -s, 0),
                vec4::unitY(),
                vec4(s, 0, c, 0),
                vec4::unitW()
            };
        }

        static mat rotZ(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                vec4(c, s, 0, 0),
                vec4(-s, c, 0, 0),
                vec4::unitZ(),
                vec4::unitW()
            };
        }

        [[nodiscard]] constexpr mat3 upperLeft3x3() const
        {
            return {
                vec3(col(0)[0], col(0)[1], col(0)[2]),
                vec3(col(1)[0], col(1)[1], col(1)[2]),
                vec3(col(2)[0], col(2)[1], col(2)[2]),
            };
        }

        // fast inverse computation for affine transformation when scaling is not present
        // in that case the linear part is orthogonal which means that its inverse is transposed
        // and greatly simplifies the case
        [[nodiscard]] constexpr mat fastInversed() const noexcept
        {
            const auto invUpperLeft = upperLeft3x3().transposed();
            const auto newT = -invUpperLeft * vec3(col(3)[0], col(3)[1], col(3)[2]);
            return mat{
                vec4(invUpperLeft.col(0), 0.0),
                vec4(invUpperLeft.col(1), 0.0),
                vec4(invUpperLeft.col(2), 0.0),
                vec4(newT, 1.0),
            };
        }
    };

    using mat4 = mat<4, 4, cadf>;
}

#endif //CAD_MAT4_H
