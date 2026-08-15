//
// Created on 3/7/26.
//

#ifndef CAD_VEC2_H
#define CAD_VEC2_H

#include <array>
#include <span>

#include <cad_math/VecBase.hpp>
#include <cad_math/Common.hpp>

namespace cadm {
    template <typename T>
    struct Vec<T, 2> : VecBase<T, 2, Vec<T, 2>> {
        union {
            struct {
                T x, y;
            };

            struct {
                T r, g;
            };

            std::array<T, 2> data;
        };

        constexpr Vec(const std::initializer_list<T> values)
        : data{} {
            std::size_t i = 0;
            for (const T v : values) {
                if (i == 2) {
                    break;
                }
                data[i++] = v;
            }
        }

        explicit constexpr Vec(T v)
        : x(v),
          y(v) {}

        constexpr Vec()
        : x(T{0}),
          y(T{0}) {}

        constexpr Vec(const T x, const T y)
        : x(x),
          y(y) {}

        constexpr static Vec unitX() noexcept {
            return {T{1}, T{0}};
        }

        constexpr static Vec unitY() noexcept {
            return {T{0}, T{1}};
        }

        [[nodiscard]] constexpr std::span<T, 2> flatView() const {
            return data;
        }

        /// @brief Component indices for operator[] access
        struct Index {
            /// @brief X component
            static constexpr std::size_t X = 0;

            /// @brief Y component
            static constexpr std::size_t Y = 1;
        };
    };

    using Vec2 = Vec<cadf, 2>;
    using Vec2I = Vec<int, 2>;
}
#endif //CAD_VEC2_H
