//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MATBASE_H
#define CAD_MATBASE_H

#include <cstddef>
#include <cad_math/common.h>
#include <cmath>
#include <algorithm>

namespace cadm
{
    enum class layout { row_major, column_major };

    template <typename Derived, std::size_t R, std::size_t C, typename T, layout L = layout::column_major>
    struct mat_base
    {
        static constexpr std::size_t index(const std::size_t row, const std::size_t col) noexcept
        {
            if constexpr (L == layout::row_major)
            {
                return row * C + col;
            }
            else
            {
                return col * R + row;
            }
        }

        constexpr T& operator()(const std::size_t row, const std::size_t col) noexcept
        {
            return static_cast<Derived*>(this)->data[index(row, col)];
        }

        constexpr const T& operator()(const std::size_t row, const std::size_t col) const noexcept
        {
            return static_cast<const Derived*>(this)->data[index(row, col)];
        }

        constexpr T& operator[](const std::size_t i) noexcept
        {
            return static_cast<Derived*>(this)->data[i];
        }

        constexpr const T& operator[](const std::size_t i) const noexcept
        {
            return static_cast<const Derived*>(this)->data[i];
        }

        friend constexpr bool operator==(const Derived& lhs, const Derived& rhs)
        {
            for (std::size_t i = 0; i < R * C; ++i)
            {
                auto diff = lhs[i] - rhs[i];
                if (diff < -eps || diff > eps)
                    return false;
            }
            return true;
        }

        friend constexpr Derived operator+(Derived lhs, const Derived& rhs)
        {
            for (std::size_t i = 0; i < R * C; ++i) lhs[i] += rhs[i];
            return lhs;
        }

        friend constexpr Derived operator-(Derived lhs, const Derived& rhs)
        {
            for (std::size_t i = 0; i < R * C; ++i) lhs[i] -= rhs[i];
            return lhs;
        }

        constexpr Derived operator*(const Derived& rhs) const requires (R == C)
        {
            Derived M{};
            for (std::size_t i = 0; i < R; ++i)
            {
                for (std::size_t j = 0; j < C; ++j)
                {
                    for (std::size_t k = 0; k < R; ++k)
                    {
                        M(i, j) += (*this)(i, k) * rhs(k, j);
                    }
                }
            }
            return M;
        }

        // TODO: implement transposition for non square matrices
        // TODO: improve implementation of transpose for better performance

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

        [[nodiscard]] constexpr Derived transposed() const requires (R == C)
        {
            Derived result{};
            for (std::size_t i = 0; i < R; ++i)
            {
                for (std::size_t j = 0; j < C; ++j)
                {
                    result(j, i) = (*this)(i, j);
                }
            }
            return result;
        }

        // TODO: refactor this method to swap rows with additional structures
        //  (implement additional rows and columns structures referencing the data inside the matrix)
        constexpr Derived inverse() const requires (R == C)
        {
            Derived temp = *static_cast<const Derived*>(this);
            Derived inv = Derived::identity();

            for (std::size_t i = 0; i < R; ++i)
            {
                // Pivot
                std::size_t pivot = i;
                for (std::size_t j = i + 1; j < R; ++j)
                {
                    if (std::abs(temp(j, i)) > std::abs(temp(pivot, i)))
                    {
                        pivot = j;
                    }
                }

                if (std::abs(temp(pivot, i)) < eps)
                {
                    // Matrix is singular
                    return Derived{};
                }

                // Swap rows
                if (pivot != i)
                {
                    for (std::size_t k = 0; k < C; ++k)
                    {
                        std::swap(temp(i, k), temp(pivot, k));
                        std::swap(inv(i, k), inv(pivot, k));
                    }
                }

                // Normalize row i
                T div = temp(i, i);
                for (std::size_t k = 0; k < C; ++k)
                {
                    temp(i, k) /= div;
                    inv(i, k) /= div;
                }

                // Eliminate other rows
                for (std::size_t j = 0; j < R; ++j)
                {
                    if (i != j)
                    {
                        T mul = temp(j, i);
                        for (std::size_t k = 0; k < C; ++k)
                        {
                            temp(j, k) -= mul * temp(i, k);
                            inv(j, k) -= mul * inv(i, k);
                        }
                    }
                }
            }

            return inv;
        }
    };
}

#endif //CAD_MATBASE_H
