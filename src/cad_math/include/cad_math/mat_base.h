//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MATBASE_H
#define CAD_MATBASE_H
#include <cstddef>
#include <cad_math/common.h>

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
    };
}

#endif //CAD_MATBASE_H
