//
// Created on 3/7/26.
//

#ifndef CAD_RAY4_H
#define CAD_RAY4_H

#include "vec4.h"
#include "mat4.h"

namespace cadm
{
    struct ray4
    {
        vec4 origin;
        vec4 direction;

        constexpr ray4(const vec4& point, const vec4& direction) : origin(point), direction(direction)
        {
        }

        friend constexpr bool operator==(const ray4& a, const ray4& b)
        {
            return a.direction == b.direction && a.origin == b.origin;
        }

        friend constexpr bool operator!=(const ray4& a, const ray4& b)
        {
            return !(a == b);
        }

        constexpr ray4& operator*=(const mat4& m)
        {
            origin = m * origin;
            direction = m * direction;
            return *this;
        }

        friend constexpr ray4 operator*(const mat4& m, ray4 r)
        {
            r *= m;
            return r;
        }

        friend constexpr ray4 operator*(ray4 r, const mat4& m)
        {
            return m * r;
        }
    };
}

#endif //CAD_RAY4_H
