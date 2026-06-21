//
// Created on 3/7/26.
//

#ifndef CAD_VEC2I_H
#define CAD_VEC2I_H

#include <array>

#include "VecBase.hpp"

namespace cadm {
    template <>
    struct Vec<2, int> : VecBase<Vec<2, int>, 2, int> {
        union {
            struct {
                int x, y;
            };

            struct {
                int r, g;
            };

            std::array<int, 2> data;
        };

        constexpr Vec() : x(0), y(0) {}

        constexpr Vec(const int x, const int y) : x(x), y(y) {}

        constexpr static Vec unitX() noexcept {
            return {1, 0};
        }

        constexpr static Vec unitY() noexcept {
            return {0, 1};
        }
    };

    using Vec2I = Vec<2, int>;
}

#endif //CAD_VEC2I_H