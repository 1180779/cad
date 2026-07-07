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
    struct Vec<4, T> : VecBase<Vec<4, T>, 4, T> {
        union {
            struct {
                T x, y, z, w;
            };

            struct {
                T r, g, b, a;
            };

            std::array<T, 4> data;
        };

        constexpr Vec() : x(0), y(0), z(0), w(0) {}

        constexpr Vec(const T x, const T y, const T z, const T w) : x(x), y(y), z(z), w(w) {}

        constexpr Vec(const Vec3 &v, const T w) : x(v.x), y(v.y), z(v.z), w(w) {}

        constexpr Vec(const T x, const Vec3 &v) : x(x), y(v.x), z(v.y), w(v.z) {}

        [[nodiscard]] constexpr Vec cross(const Vec &other) const {
            return {x * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x, 0};
        }

        [[nodiscard]] constexpr Vec3 xyz() const {
            return {x, y, z};
        }

        constexpr static Vec unitX() noexcept {
            return {1.0, 0.0, 0.0, 0.0};
        }

        constexpr static Vec unitY() noexcept {
            return {0.0, 1.0, 0.0, 0.0};
        }

        constexpr static Vec unitZ() noexcept {
            return {0.0, 0.0, 1.0, 0.0};
        }

        constexpr static Vec unitW() noexcept {
            return {0.0, 0.0, 0.0, 1.0};
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

    using vec4 = Vec<4, cadf>;
}

#endif //CAD_VEC4_H
