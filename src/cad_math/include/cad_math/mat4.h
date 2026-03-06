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
    struct mat4 : public mat_base<mat4, 4, 4, cadf>
    {
        union
        {
            struct
            {
                cadf m00, m10, m20, m30,
                     m01, m11, m21, m31,
                     m02, m12, m22, m32,
                     m03, m13, m23, m33;
            };

            cadf data[16]{};
        };

    public:
        using mat_base<mat4, 4, 4, cadf>::operator*;

        mat4()
        {
            m00 = m01 = m02 = m03 = m10 = m11 = m12 = m13 = m20 = m21 = m22 = m23 = m30 = m31 = m32 = m33 = 0.0f;
        }

        mat4(const cadf m00, const cadf m01, const cadf m02, const cadf m03,
             const cadf m10, const cadf m11, const cadf m12, const cadf m13,
             const cadf m20, const cadf m21, const cadf m22, const cadf m23,
             const cadf m30, const cadf m31, const cadf m32, const cadf m33)
        {
            this->m00 = m00;
            this->m01 = m01;
            this->m02 = m02;
            this->m03 = m03;

            this->m10 = m10;
            this->m11 = m11;
            this->m12 = m12;
            this->m13 = m13;

            this->m20 = m20;
            this->m21 = m21;
            this->m22 = m22;
            this->m23 = m23;

            this->m30 = m30;
            this->m31 = m31;
            this->m32 = m32;
            this->m33 = m33;
        }

        mat4(const vec4& r0, const vec4& r1, const vec4& r2, const vec4& r3)
        {
            this->m00 = r0.x;
            this->m01 = r0.y;
            this->m02 = r0.z;
            this->m03 = r0.w;

            this->m10 = r1.x;
            this->m11 = r1.y;
            this->m12 = r1.z;
            this->m13 = r1.w;

            this->m20 = r2.x;
            this->m21 = r2.y;
            this->m22 = r2.z;
            this->m23 = r2.w;

            this->m30 = r3.x;
            this->m31 = r3.y;
            this->m32 = r3.z;
            this->m33 = r3.w;
        }

        static mat4 identity()
        {
            return mat4{
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        }

        // useful article to revisit view matrix:
        // https://www.3dgep.com/understanding-the-view-matrix/

        // Look at matrix for Right Handed coordinate system
        static mat4 lookAtRH(const vec3& eye, const vec3& target, const vec3& up)
        {
            const vec3 zAxis = (eye - target).normalized(); // forward
            const vec3 xAxis = up.cross(zAxis).normalized(); // right
            const vec3 yAxis = zAxis.cross(xAxis); // up

            return {
                vec4(xAxis.x, xAxis.y, xAxis.z, -xAxis.dot(eye)),
                vec4(yAxis.x, yAxis.y, yAxis.z, -yAxis.dot(eye)),
                vec4(zAxis.x, zAxis.y, zAxis.z, -zAxis.dot(eye)),
                vec4(0.0, 0.0, 0.0, 1.0),
            };
        }

        // static mat4 perspective(const cadf fov, const cadf aspect, const cadf near, const cadf far)
        // {
        //     const cadf tanHalfFov = std::tan(fov / 2.0f);
        //
        //     mat4 result{};
        //     result(0, 0) = 1.0f / (aspect * tanHalfFov);
        //     result(1, 1) = 1.0f / tanHalfFov;
        //     result(2, 2) = -(far + near) / (far - near);
        //     result(2, 3) = -1.0f;
        //     result(3, 2) = -(2.0f * far * near) / (far - near);
        //     return result;
        // }
        //
        static mat4 ortho(const cadf left, const cadf right, const cadf bottom, const cadf top, const cadf near,
                          const cadf far)
        {
            return {
                vec4(2.0 / (right - left), 0.0, 0.0, -(right + left) / (right - left)),
                vec4(0.0, 2.0 / (top - bottom), 0.0, -(top + bottom) / (top - bottom)),
                vec4(0.0, 0.0, -2.0 / (far - near), -(far + near) / (far - near)),
                vec4(0.0, 0.0, 0.0, 1.0)
            };
        }

        static mat4 scale(const vec3& s) { return scale(s.x, s.y, s.z); }

        static mat4 scale(const cadf sx, const cadf sy, const cadf sz)
        {
            return diag(sx, sy, sz, 1.0);
        }

        static mat4 diag(const cadf m0, const cadf m1, const cadf m2, const cadf m3)
        {
            return mat4{
                m0, 0, 0, 0,
                0, m1, 0, 0,
                0, 0, m2, 0,
                0, 0, 0, m3
            };
        }

        static mat4 translation(const vec3& t) { return translation(t.x, t.y, t.z); }

        static mat4 translation(const cadf tx, const cadf ty, const cadf tz)
        {
            return mat4{
                1, 0, 0, tx,
                0, 1, 0, ty,
                0, 0, 1, tz,
                0, 0, 0, 1
            };
        }

        static mat4 rotX(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return mat4{
                1, 0, 0, 0,
                0, c, -s, 0,
                0, s, c, 0,
                0, 0, 0, 1
            };
        }

        static mat4 rotY(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return mat4{
                c, 0, s, 0,
                0, 1, 0, 0,
                -s, 0, c, 0,
                0, 0, 0, 1
            };
        }

        static mat4 rotZ(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return mat4{
                c, -s, 0, 0,
                s, c, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        }

        vec4 operator*(const vec4& v) const
        {
            return {
                (*this)(0, 0) * v.x + (*this)(0, 1) * v.y + (*this)(0, 2) * v.z + (*this)(0, 3) * v.w,
                (*this)(1, 0) * v.x + (*this)(1, 1) * v.y + (*this)(1, 2) * v.z + (*this)(1, 3) * v.w,
                (*this)(2, 0) * v.x + (*this)(2, 1) * v.y + (*this)(2, 2) * v.z + (*this)(2, 3) * v.w,
                (*this)(3, 0) * v.x + (*this)(3, 1) * v.y + (*this)(3, 2) * v.z + (*this)(3, 3) * v.w
            };
        }

        [[nodiscard]] mat3 upperLeft3x3() const
        {
            return {m00, m01, m02, m10, m11, m12, m20, m21, m22};
        }

        // fast inverse computation for affine transformation when scaling is not present
        // in that case the linear part is orthogonal which means that its inverse is transposed
        // and greatly simplifies the case
        [[nodiscard]] mat4 fastInversed() const noexcept
        {
            const auto invUpperLeft = upperLeft3x3().transposed();
            const auto newT = -invUpperLeft * vec3(m03, m13, m23);
            return mat4{
                vec4(invUpperLeft.m00, invUpperLeft.m10, invUpperLeft.m20, newT.x),
                vec4(invUpperLeft.m01, invUpperLeft.m11, invUpperLeft.m21, newT.y),
                vec4(invUpperLeft.m02, invUpperLeft.m12, invUpperLeft.m22, newT.z),
                vec4(0.0, 0.0, 0.0, 1.0),
            };
        }
    };
}

#endif //CAD_MAT4_H
