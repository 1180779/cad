//
// Created by Radosław Głasek on 02.08.2026
//

#ifndef CAD_MAT2_HXX
#define CAD_MAT2_HXX

#include "Common.hpp"
#include "MatBase.hpp"
#include "Vec2.hpp"

namespace cadm {
    template <typename T, std::size_t R, std::size_t C>
    struct Mat;

    template <typename T>
    struct Mat<T, 2, 2> : MatBase<T, 2, 2, Vec2, Vec2, Mat, Mat<T, 2, 2>> {
        union {
            T data[4]{};
            Vec<T, 2> columns[2];
        };

        constexpr Mat() {
            for (auto &cell : data) {
                cell = 0;
            }
        }

        constexpr Mat(const T x0, const T x1, const T y0, const T y1) {
            columns[0] = Vec2{x0, x1};
            columns[1] = Vec2{y0, y1};
        }

        constexpr Mat(const Vec2 &c0, const Vec2 &c1) {
            columns[0] = c0;
            columns[1] = c1;
        }

        constexpr static Mat identity() {
            return {T{1}, T{0}, T{0}, T{1}};
        }

        constexpr static Mat diag(const T m0, const T m1) {
            return {m0, T{0}, T{0}, m1};
        }

        /// @brief Symmetric matrix from its three distinct entries <tt>[m00
        /// m01; m01 m11]</tt>
        constexpr static Mat symmetric(const T m00, const T m01, const T m11) {
            return {m00, m01, m01, m11};
        }

        [[nodiscard]] constexpr T determinant() const noexcept {
            return data[0] * data[3] - data[2] * data[1];
        }

        /// @brief Solve <tt>this * x = b</tt> with Cramer's rule
        /// @returns <tt>std::nullopt</tt> if (near-)singular
        [[nodiscard]] constexpr std::optional<Vec2> solveCramer(const Vec2 &b) const noexcept {
            const T det = determinant();
            if (std::abs(det) < gc_eps) {
                return std::nullopt;
            }
            return Vec2{
                (b.x * data[3] - b.y * data[2]) / det,
                (data[0] * b.y - data[1] * b.x) / det,
            };
        }
    };

    using Mat2 = Mat<cadf, 2, 2>;
}

#endif //CAD_MAT2_HXX
