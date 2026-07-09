//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_VEC4_H
#define CAD_VEC4_H

#include <array>

#include "VecBase.hpp"
#include "Vec3.hpp"

namespace cadm {
    template <typename T>
    struct Vec<T, 4> : VecBase<T, 4, Vec<T, 4>> {
        union {
            struct {
                T x, y, z, w;
            };

            struct {
                T r, g, b, a;
            };

            std::array<T, 4> data;
        };

        constexpr Vec(const std::initializer_list<T> values)
        : data{} {
            std::size_t i = 0;
            for (const T v : values) {
                if (i == 4) {
                    break;
                }
                data[i++] = v;
            }
        }

        explicit constexpr Vec(T v)
        : x{v},
          y{v},
          z{v},
          w{v} {}

        constexpr Vec()
        : x(T{0}),
          y(T{0}),
          z(T{0}),
          w(T{0}) {}

        constexpr Vec(const T x, const T y, const T z, const T w)
        : x(x),
          y(y),
          z(z),
          w(w) {}

        constexpr Vec(const Vec3 &v, const T w)
        : x(v.x),
          y(v.y),
          z(v.z),
          w(w) {}

        constexpr Vec(const T x, const Vec3 &v)
        : x(x),
          y(v.x),
          z(v.y),
          w(v.z) {}

        [[nodiscard]] constexpr Vec cross(const Vec &other) const {
            return {x * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x, 0};
        }

        [[nodiscard]] constexpr std::span<const T, 4> flatView() const {
            return data;
        }

        [[nodiscard]] constexpr Vec3 xyz() const {
            return {x, y, z};
        }

        constexpr static Vec unitX() noexcept {
            return {T{1}, T{0}, T{0}, T{0}};
        }

        constexpr static Vec unitY() noexcept {
            return {T{0}, T{1}, T{0}, T{0}};
        }

        constexpr static Vec unitZ() noexcept {
            return {T{0}, T{0}, T{1}, T{0}};
        }

        constexpr static Vec unitW() noexcept {
            return {T{0}, T{0}, T{0}, T{1}};
        }

        /// @brief Component indices for operator[] access
        struct Index {
            /// @brief X component
            static constexpr std::size_t X = 0;

            /// @brief Y component
            static constexpr std::size_t Y = 1;

            /// @brief Z component
            static constexpr std::size_t Z = 2;

            /// @brief W component
            static constexpr std::size_t W = 3;
        };
    };

    using Vec4 = Vec<cadf, 4>;
    using Vec4I = Vec<int, 4>;
}

#endif //CAD_VEC4_H
