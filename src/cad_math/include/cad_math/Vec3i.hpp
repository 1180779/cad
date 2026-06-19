//
// Created on 3/3/26.
//

#ifndef CAD_VEC3I_H
#define CAD_VEC3I_H

#include <array>

#include "VecBase.hpp"

namespace cadm {
    template <>
    struct Vec<3, int> : VecBase<Vec<3, int>, 3, int> {
        union {
            struct {
                int x, y, z;
            };

            struct {
                int r, g, b;
            };

            std::array<int, 3> data;
        };

        constexpr Vec() : x(0), y(0), z(0) {}

        constexpr Vec(const int x, const int y, const int z) : x(x), y(y), z(z) {}

        constexpr static Vec unitX() noexcept {
            return {1, 0, 0};
        }

        constexpr static Vec unitY() noexcept {
            return {0, 1, 0};
        }

        constexpr static Vec unitZ() noexcept {
            return {0, 0, 1};
        }
    };

    using vec3i = Vec<3, int>;
}

#endif //CAD_VEC3I_H