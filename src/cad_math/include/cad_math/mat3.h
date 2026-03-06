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
    struct mat3 : public mat_base<mat3, 3, 3, cadf>
    {
        union
        {
            struct
            {
                // column-major
                cadf m00, m10, m20,
                     m01, m11, m21,
                     m02, m12, m22;
            };

            cadf data[9]{};
        };

    public:
        using mat_base<mat3, 3, 3, cadf>::operator*;

        mat3()
        {
            m00 = m01 = m02 = m10 = m11 = m12 = m20 = m21 = m22 = 0.0f;
        }

        mat3(const cadf m00, const cadf m01, const cadf m02,
             const cadf m10, const cadf m11, const cadf m12,
             const cadf m20, const cadf m21, const cadf m22)
        {
            this->m00 = m00;
            this->m01 = m01;
            this->m02 = m02;

            this->m10 = m10;
            this->m11 = m11;
            this->m12 = m12;

            this->m20 = m20;
            this->m21 = m21;
            this->m22 = m22;
        }

        mat3(const vec3& c0, const vec3& c1, const vec3& c2)
        {
            this->m00 = c0.x;
            this->m01 = c0.y;
            this->m02 = c0.z;

            this->m10 = c1.x;
            this->m11 = c1.y;
            this->m12 = c1.z;

            this->m20 = c2.x;
            this->m21 = c2.y;
            this->m22 = c2.z;
        }

        static mat3 identity()
        {
            return mat3{
                1, 0, 0,
                0, 1, 0,
                0, 0, 1,
            };
        }

        static mat3 scale(const vec2& s) { return scale(s.x, s.y); }

        static mat3 scale(const cadf sx, const cadf sy)
        {
            return diag(sx, sy, 1.0);
        }

        static mat3 diag(const cadf m0, const cadf m1, const cadf m2)
        {
            return mat3{
                m0, 0, 0,
                0, m1, 0,
                0, 0, m2,
            };
        }

        static mat3 translation(const vec2& t) { return translation(t.x, t.y); }

        static mat3 translation(const cadf tx, const cadf ty)
        {
            return mat3{
                1, 0, tx,
                0, 1, ty,
                0, 0, 1
            };
        }
        static mat3 rotX(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return mat3{
                1, 0, 0,
                0, c, -s,
                0, s, c,
            };
        }

        static mat3 rotY(const cadf alpha)
        {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return mat3{
                c, 0, s,
                0, 1, 0,
                -s, 0, c,
            };
        }

        vec3 operator*(const vec3& v) const
        {
            return {
                (*this)(0, 0) * v.x + (*this)(0, 1) * v.y + (*this)(0, 2) * v.z,
                (*this)(1, 0) * v.x + (*this)(1, 1) * v.y + (*this)(1, 2) * v.z,
                (*this)(2, 0) * v.x + (*this)(2, 1) * v.y + (*this)(2, 2) * v.z,
            };
        }
    };
}
#endif //CAD_MAT3_H