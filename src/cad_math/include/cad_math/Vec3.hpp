//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_VEC3_H
#define CAD_VEC3_H

#include <array>

#include "VecBase.hpp"

namespace cadm {
    template <>
    struct Vec<3, cadf> : VecBase<Vec<3, cadf>, 3, cadf> {
        union {
            struct {
                cadf x, y, z;
            };

            struct {
                cadf r, g, b;
            };

            std::array<cadf, 3> data;
        };

        constexpr Vec() : x(0), y(0), z(0) {}

        constexpr Vec(const cadf x, const cadf y, const cadf z) : x(x), y(y), z(z) {}

        constexpr static Vec unitX() noexcept {
            return {1.0, 0.0, 0.0};
        }

        constexpr static Vec unitY() noexcept {
            return {0.0, 1.0, 0.0};
        }

        constexpr static Vec unitZ() noexcept {
            return {0.0, 0.0, 1.0};
        }

        [[nodiscard]] constexpr Vec cross(const Vec &other) const {
            return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
        }

        /// @brief Component indices for operator[] access
        struct Index {
            /// @brief X component
            static constexpr std::size_t X = 0;

            /// @brief Y component
            static constexpr std::size_t Y = 1;

            /// @brief Z component
            static constexpr std::size_t Z = 2;
        };
    };

    using Vec3 = Vec<3, cadf>;
}

#endif //CAD_VEC3_H
