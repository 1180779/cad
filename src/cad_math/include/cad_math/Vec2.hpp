//
// Created on 3/7/26.
//

#ifndef CAD_VEC2_H
#define CAD_VEC2_H

#include <array>

#include <cad_math/VecBase.hpp>
#include <cad_math/Common.hpp>

namespace cadm {
    template <typename T>
    struct Vec<2, T> : VecBase<Vec<2, T>, 2, T> {
        union {
            struct {
                T x, y;
            };

            struct {
                T r, g;
            };

            std::array<T, 2> data;
        };

        constexpr Vec() : x(0), y(0) {}

        constexpr Vec(const T x, const T y) : x(x), y(y) {}

        constexpr static Vec unitX() noexcept {
            return {1.0, 0.0};
        }

        constexpr static Vec unitY() noexcept {
            return {0.0, 1.0};
        }

        /// @brief Component indices for operator[] access
        struct Index {
            /// @brief X component
            static constexpr std::size_t X = 0;

            /// @brief Y component
            static constexpr std::size_t Y = 1;
        };
    };

    using vec2 = Vec<2, cadf>;
}
#endif //CAD_VEC2_H
