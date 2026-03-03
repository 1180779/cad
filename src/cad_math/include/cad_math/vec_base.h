//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MATH_H
#define CAD_MATH_H
#include <cassert>
#include <cad_math/common.h>

namespace cadm
{
    template <typename Derived, std::size_t N, typename T>
    struct vec_base
    {
        constexpr T& operator[](std::size_t i) noexcept { return static_cast<Derived*>(this)->data[i]; }

        constexpr const T& operator[](std::size_t i) const noexcept
        {
            return static_cast<const Derived*>(this)->data[i];
        }

        T& at(std::size_t i)
        {
            assert(i < N);
            return (*this)[i];
        }

        // vector-vector operators

        friend constexpr bool operator==(const Derived& lhs, const Derived& rhs) noexcept
        {
            for (int i = 0; i < N; ++i)
            {
                auto diff = lhs[i] - rhs[i];
                if (diff < -eps || diff > -eps)
                    return false;
            }
            return true;
        }

        friend constexpr bool operator!=(const Derived& lhs, const Derived& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        friend constexpr Derived operator+(Derived lhs, const Derived& rhs)
        {
            for (int i = 0; i < N; ++i)
                lhs.data[i] += rhs.data[i];
            return lhs;
        }

        friend constexpr Derived operator-(Derived lhs, const Derived& rhs)
        {
            for (int i = 0; i < N; ++i)
                lhs.data[i] -= rhs.data[i];
            return lhs;
        }

        friend constexpr Derived operator*(Derived lhs, const Derived& rhs)
        {
            for (int i = 0; i < N; ++i)
                lhs.data[i] *= rhs.data[i];
            return lhs;
        }

        friend constexpr Derived operator/(Derived lhs, const Derived& rhs)
        {
            for (int i = 0; i < N; ++i)
                lhs.data[i] /= rhs.data[i];
            return lhs;
        }

        // vector-scalar operators

        friend constexpr Derived operator+(Derived lhs, const cadf s)
        {
            for (int i = 0; i < N; ++i)
                lhs[i] += s;
            return lhs;
        }

        friend constexpr Derived operator+(const cadf lhs, Derived rhs) { return lhs + rhs; }

        friend constexpr Derived operator-(Derived lhs, const cadf s)
        {
            for (int i = 0; i < N; ++i)
                lhs[i] -= s;
            return lhs;
        }

        friend constexpr Derived operator*(Derived lhs, const cadf s)
        {
            for (int i = 0; i < N; ++i)
                lhs[i] *= s;
            return lhs;
        }

        friend constexpr Derived operator*(const cadf lhs, const Derived rhs) { return rhs * lhs; }

        friend constexpr Derived operator/(Derived lhs, const cadf s)
        {
            for (int i = 0; i < N; ++i)
                lhs[i] /= s;
            return lhs;
        }

        // common methods

        void normalize() noexcept
        {
            const auto lengthSq  = lengthSquared();
            if (std::abs(lengthSquared() - 1.0) < eps)
                return;

            const auto length = std::sqrt(lengthSq);
            for (int i = 0; i < N; ++i)
                (*this)[i] /= length;
        }

        constexpr Derived normalized() const noexcept
        {
            Derived res = *this;
            res.normalize();
            return res;
        }

        constexpr T length() const noexcept
        {
            T res = lengthSquared();
            return std::sqrt(res);
        }

        constexpr T lengthSquared() const noexcept
        {
            T res{};
            for (int i = 0; i < N; ++i)
                res += (*this)[i] * (*this)[i];
            return res;
        }

        constexpr T dot(const Derived& other) const noexcept
        {
            T res = 0;
            for (int i = 0; i < N; ++i)
                res += other.data[i] * (*this)[i];
            return res;
        }
    };
}
#endif //CAD_MATH_H
