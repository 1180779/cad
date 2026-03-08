//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MATBASE_H
#define CAD_MATBASE_H

#include <cstddef>
#include <cmath>
#include <optional>

#include "common.h"

namespace cadm
{
    template <typename Derived, typename RowType, std::size_t C, typename T>
    struct mat_row_ref
    {
        Derived& matrix;
        std::size_t rowIdx;

        constexpr mat_row_ref(Derived& mat, const std::size_t idx) noexcept
            : matrix(mat), rowIdx(idx)
        {
        }

        constexpr T& operator[](std::size_t col) noexcept
        {
            return matrix(rowIdx, col);
        }

        constexpr const T& operator[](std::size_t col) const noexcept
        {
            return matrix(rowIdx, col);
        }

        constexpr operator RowType() const noexcept
        {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k)
            {
                res[k] = (*this)[k];
            }
            return res;
        }

        constexpr mat_row_ref& operator=(const RowType& vec) noexcept
        {
            for (std::size_t k = 0; k < C; ++k)
            {
                (*this)[k] = vec[k];
            }
            return *this;
        }

        constexpr void swap(mat_row_ref other) noexcept
        {
            for (std::size_t k = 0; k < C; ++k)
            {
                std::swap((*this)[k], other[k]);
            }
        }

        // ===== compound assignment operators (modify in-place) =====

        constexpr mat_row_ref& operator+=(const RowType& vec) noexcept
        {
            for (std::size_t k = 0; k < C; ++k)
            {
                (*this)[k] += vec[k];
            }
            return *this;
        }

        constexpr mat_row_ref& operator+=(const mat_row_ref& other) noexcept
        {
            for (std::size_t k = 0; k < C; ++k)
            {
                (*this)[k] += other[k];
            }
            return *this;
        }

        constexpr mat_row_ref& operator-=(const RowType& vec) noexcept
        {
            for (std::size_t k = 0; k < C; ++k)
            {
                (*this)[k] -= vec[k];
            }
            return *this;
        }

        constexpr mat_row_ref& operator-=(const mat_row_ref& other) noexcept
        {
            for (std::size_t k = 0; k < C; ++k)
            {
                (*this)[k] -= other[k];
            }
            return *this;
        }

        constexpr mat_row_ref& operator*=(T scalar) noexcept
        {
            for (std::size_t k = 0; k < C; ++k)
            {
                (*this)[k] *= scalar;
            }
            return *this;
        }

        constexpr mat_row_ref& operator/=(T scalar) noexcept
        {
            for (std::size_t k = 0; k < C; ++k)
            {
                (*this)[k] /= scalar;
            }
            return *this;
        }

        // ===== binary operators (return new vector, don't modify) =====

        constexpr RowType operator+(const RowType& vec) const noexcept
        {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k)
            {
                res[k] = (*this)[k] + vec[k];
            }
            return res;
        }

        constexpr RowType operator+(const mat_row_ref& other) const noexcept
        {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k)
            {
                res[k] = (*this)[k] + other[k];
            }
            return res;
        }

        constexpr RowType operator-(const RowType& vec) const noexcept
        {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k)
            {
                res[k] = (*this)[k] - vec[k];
            }
            return res;
        }

        constexpr RowType operator-(const mat_row_ref& other) const noexcept
        {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k)
            {
                res[k] = (*this)[k] - other[k];
            }
            return res;
        }

        constexpr RowType operator*(T scalar) const noexcept
        {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k)
            {
                res[k] = (*this)[k] * scalar;
            }
            return res;
        }

        friend constexpr RowType operator*(T scalar, const mat_row_ref& row) noexcept
        {
            return row * scalar;
        }

        constexpr RowType operator/(T scalar) const noexcept
        {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k)
            {
                res[k] = (*this)[k] / scalar;
            }
            return res;
        }
    };

    template <template <std::size_t, std::size_t, typename> class MatrixT, typename Derived, typename ColType, typename
              RowType, std::size_t R, std::size_t C, typename T>
    struct mat_base
    {
        constexpr ColType& col(const std::size_t i) noexcept
        {
            return static_cast<Derived*>(this)->columns[i];
        }

        constexpr const ColType& col(const std::size_t i) const noexcept
        {
            return static_cast<const Derived*>(this)->columns[i];
        }

        constexpr RowType row(const std::size_t i) const noexcept
        {
            RowType res{};
            for (std::size_t j = 0; j < C; ++j)
            {
                res[j] = (*this)(i, j);
            }
            return res;
        }

        constexpr T& operator()(const std::size_t row, const std::size_t col) noexcept
        {
            return this->col(col)[row];
        }

        constexpr const T& operator()(const std::size_t row, const std::size_t col) const noexcept
        {
            return this->col(col)[row];
        }

        friend constexpr Derived operator-(Derived lhs)
        {
            for (std::size_t i = 0; i < C; ++i)
                lhs.col(i) = -lhs.col(i);
            return lhs;
        }

        friend constexpr bool operator==(const Derived& lhs, const Derived& rhs)
        {
            for (std::size_t i = 0; i < C; ++i)
            {
                if (lhs.col(i) != rhs.col(i))
                    return false;
            }
            return true;
        }

        friend constexpr Derived operator+(Derived lhs, const Derived& rhs)
        {
            for (std::size_t i = 0; i < C; ++i)
                lhs.col(i) += rhs.col(i);
            return lhs;
        }

        friend constexpr Derived operator-(Derived lhs, const Derived& rhs)
        {
            for (std::size_t i = 0; i < C; ++i)
                lhs.col(i) -= rhs.col(i);
            return lhs;
        }

        template <typename OtherDerived, typename OtherCol, typename OtherRow, std::size_t OtherC>
        constexpr auto operator*(const mat_base<MatrixT, OtherDerived, OtherCol, OtherRow, C, OtherC, T>& rhs) const
        {
            using ResultType = MatrixT<R, OtherC, T>;
            ResultType M{};

            for (std::size_t i = 0; i < R; ++i)
            {
                const auto r = this->row(i);
                for (std::size_t j = 0; j < OtherC; ++j)
                {
                    M(i, j) = r.dot(rhs.col(j));
                }
            }
            return M;
        }

        constexpr ColType operator*(const RowType& v) const noexcept
        {
            ColType res = this->col(0) * v[0];
            for (std::size_t j = 1; j < C; ++j)
            {
                res += this->col(j) * v[j];
            }
            return res;
        }

        constexpr void transpose() requires (R == C)
        {
            for (std::size_t i = 0; i < R; ++i)
            {
                for (std::size_t j = i + 1; j < C; ++j)
                {
                    std::swap((*this)(i, j), (*this)(j, i));
                }
            }
        }

        [[nodiscard]] constexpr auto transposed() const
        {
            using ResultType = MatrixT<C, R, T>;
            ResultType result{};
            for (std::size_t i = 0; i < R; ++i)
            {
                result.col(i) = this->row(i);
            }
            return result;
        }

        constexpr auto makeRowRef(Derived& matrix, std::size_t row_idx) const noexcept
        {
            return mat_row_ref<Derived, RowType, C, T>(matrix, row_idx);
        }

        [[nodiscard]] constexpr std::optional<Derived> inverseSafe() const requires (R == C)
        {
            Derived temp = *static_cast<const Derived*>(this);
            Derived inv = Derived::identity();

            for (std::size_t i = 0; i < R; ++i)
            {
                // find pivot
                std::size_t pivot = i;
                for (std::size_t j = i + 1; j < R; ++j)
                {
                    if (std::abs(temp(j, i)) > std::abs(temp(pivot, i)))
                    {
                        pivot = j;
                    }
                }

                // check if the matrix is singular
                if (std::abs(temp(pivot, i)) < eps)
                {
                    return std::nullopt;
                }

                // swap rows if needed
                if (pivot != i)
                {
                    auto tempRowI = makeRowRef(temp, i);
                    auto tempRowPivot = makeRowRef(temp, pivot);
                    tempRowI.swap(tempRowPivot);

                    auto invRowI = makeRowRef(inv, i);
                    auto invRowPivot = makeRowRef(inv, pivot);
                    invRowI.swap(invRowPivot);
                }

                // normalize row i
                T div = temp(i, i);
                auto tempRowI = makeRowRef(temp, i);
                auto invRowI = makeRowRef(inv, i);
                tempRowI /= div;
                invRowI /= div;

                // eliminate other rows
                for (std::size_t j = 0; j < R; ++j)
                {
                    if (i != j)
                    {
                        T mul = temp(j, i);
                        auto tempRowJ = makeRowRef(temp, j);
                        auto invRowJ = makeRowRef(inv, j);
                        tempRowJ -= tempRowI * mul;
                        invRowJ -= invRowI * mul;
                    }
                }
            }

            return inv;
        }

        [[nodiscard]] constexpr Derived inverse() const requires (R == C)
        {
            if (const auto safeInverse = inverseSafe())
            {
                return safeInverse.value();
            }
            return Derived::identity();
        }
    };
}

#endif //CAD_MATBASE_H
