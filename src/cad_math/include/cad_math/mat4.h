//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MAT4_H
#define CAD_MAT4_H

#include <cad_math/mat_base.h>
#include <cad_math/vec4.h>

#include "vec_base.h"

namespace cadm
{
    struct mat4 : public mat_base<mat4, 4, 4, cadf>
    {
        union
        {
            struct
            {
                // column-major
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

        static mat4 identity()
        {
            return mat4{
                1, 0, 0, 0,
                0, 1, 0, 0,
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
    };
}

#endif //CAD_MAT4_H
