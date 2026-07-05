//
// Created by Radosław Głasek on 05.07.2026
//

#ifndef CAD_QUATERNION_HXX
#define CAD_QUATERNION_HXX

#include <span>

#include "VecBase.hpp"

namespace cadm {
    template <typename T = cadf>
    class Quaternion final {
        union {
            T data[4]{};

            struct {
                T e0, e1, e2, e3;
            };
        };

    public:
        explicit Quaternion(std::span<T, 4> d) {
            std::copy(d.begin(), d.end(), data);
        }

        explicit Quaternion(T e0, T e1, T e2, T e3) {
            this->e0 = e0;
            this->e1 = e1;
            this->e2 = e2;
            this->e3 = e3;
        }

        explicit Quaternion(T e0, const Vec<3, T> &n) {
            this->e0 = e0;
            this->e1 = n[0];
            this->e2 = n[1];
            this->e3 = n[2];
        }

        constexpr T scalarPart() noexcept {
            return e0;
        }

        constexpr Vec<3, T> vectorPart() noexcept {
            return {e1, e2, e3};
        }

        constexpr Vec<3, T> vectorPart() const noexcept {
            return {e1, e2, e3};
        }

        constexpr void setVectorPart(const Vec<3, T> &v) noexcept {
            e1 = v[0];
            e2 = v[1];
            e3 = v[2];
        }

        constexpr void setScalarPart(T s) noexcept {
            e0 = s;
        }

        constexpr T& operator[](std::size_t i) noexcept {
            return data[i];
        }

        constexpr const T& operator[](std::size_t i) const noexcept {
            return data[i];
        }

        T& at(std::size_t i) {
            assert(i < 4);
            return (*this)[i];
        }

        friend constexpr Quaternion<T> operator-(const Quaternion &v) noexcept {
            Quaternion res = static_cast<const Quaternion&>(v);
            for (int i = 0; i < 4; ++i) {
                res[i] = -res[i];
            }
            return res;
        }

        // ===== quaternion-quaternion operators =====

        friend constexpr Vec<3, T> rotateVec(const Quaternion &q, const Vec<3, T> &v) {
            // const Quaternion qv{0, v};
            // const Quaternion conj = q.conjugate();
            // const Quaternion qRot = q * qv * conj;
            // return qRot.vectorPart();
            const T q0 = q.scalarPart();
            const Vec<3, T> qv = q.vectorPart();
            return 2 * qv.dot(v) * qv + (q0 * q0 - qv.lengthSquared()) * v + 2 * q0 * qv.cross(v);
        }

        friend constexpr bool operator==(const Quaternion &lhs, const Quaternion &rhs) noexcept {
            for (int i = 0; i < 4; ++i) {
                auto diff = std::abs(lhs[i] - rhs[i]);
                if (diff > gc_eps) {
                    return false;
                }
            }
            return true;
        }

        friend constexpr bool operator!=(const Quaternion &lhs, const Quaternion &rhs) noexcept {
            return !(lhs == rhs);
        }

        constexpr static Quaternion fromRotation(T alpha, Vec<3, T> n) {
            const auto alphaO2 = alpha * 0.5;
            const auto s = std::sin(alphaO2);
            const auto c = std::cos(alphaO2);
            return {c, s * n};
        }

        constexpr Quaternion conjugate() noexcept {
            return {e0, -vectorPart()};
        }

        constexpr Quaternion inverse() {
            return conjugate() / lengthSquared();
        }

        constexpr Quaternion& operator+=(const Quaternion &rhs) {
            for (int i = 0; i < 4; ++i) {
                (*this)[i] += rhs.data[i];
            }
            return static_cast<Quaternion&>(*this);
        }

        friend constexpr Quaternion operator+(Quaternion lhs, const Quaternion &rhs) {
            lhs += rhs;
            return lhs;
        }

        constexpr Quaternion& operator-=(const Quaternion &rhs) {
            for (int i = 0; i < 4; ++i) {
                (*this)[i] -= rhs.data[i];
            }
            return static_cast<Quaternion&>(*this);
        }

        friend constexpr Quaternion operator-(Quaternion lhs, const Quaternion &rhs) {
            lhs -= rhs;
            return lhs;
        }

        constexpr Quaternion& operator*=(const Quaternion &rhs) {
            //\(w_t = (w_a w_b - x_a x_b - y_a y_b - z_a z_b)\)
            //\(x_t = (w_a x_b + x_a w_b + y_a z_b - z_a y_b)\)
            //\(y_t = (w_a y_b - x_a z_b + y_a w_b + z_a x_b)\)
            //\(z_t = (w_a z_b + x_a y_b - y_a x_b + z_a w_b)\)
            Quaternion res{
                e0 * rhs.e0 - e1 * rhs.e1 - e2 * rhs.e2 - e3 * rhs.e3,
                e0 * rhs.e1 + e1 * rhs.e0 + e2 * rhs.e3 - e3 * rhs.e2,
                e0 * rhs.e2 - e1 * rhs.e3 + e2 * rhs.e0 + e3 * rhs.e1,
                e0 * rhs.e3 + e1 * rhs.e2 - e2 * rhs.e1 + e3 * rhs.e0
            };
            *this = res;
            return *this;
        }

        friend constexpr Quaternion operator*(Quaternion lhs, const Quaternion &rhs) {
            lhs *= rhs;
            return lhs;
        }

        constexpr Quaternion& operator/=(const Quaternion &rhs) {
            for (int i = 0; i < 4; ++i) {
                (*this)[i] /= rhs.data[i];
            }
            return static_cast<Quaternion&>(*this);
        }

        friend constexpr Quaternion operator/(Quaternion lhs, const Quaternion &rhs) {
            lhs /= rhs;
            return lhs;
        }

        // ===== vector-scalar operators =====

        constexpr Quaternion& operator+=(const cadf rhs) {
            for (int i = 0; i < 4; ++i) {
                (*this)[i] += rhs;
            }
            return static_cast<Quaternion&>(*this);
        }

        friend constexpr Quaternion operator+(Quaternion lhs, const cadf s) {
            lhs += s;
            return lhs;
        }

        friend constexpr Quaternion operator+(const cadf lhs, Quaternion rhs) {
            return lhs + rhs;
        }

        constexpr Quaternion& operator-=(const cadf rhs) {
            for (int i = 0; i < 4; ++i) {
                (*this)[i] -= rhs;
            }
            return *this;
        }

        friend constexpr Quaternion operator-(Quaternion lhs, const cadf s) {
            lhs -= s;
            return lhs;
        }

        constexpr Quaternion& operator*=(const cadf rhs) {
            for (int i = 0; i < 4; ++i) {
                (*this)[i] *= rhs;
            }
            return *this;
        }

        friend constexpr Quaternion operator*(Quaternion lhs, const cadf s) {
            lhs *= s;
            return lhs;
        }

        friend constexpr Quaternion operator*(const cadf lhs, const Quaternion rhs) {
            return rhs * lhs;
        }

        constexpr Quaternion& operator/=(const cadf rhs) {
            for (int i = 0; i < 4; ++i) {
                (*this)[i] /= rhs;
            }
            return *this;
        }

        friend constexpr Quaternion operator/(Quaternion lhs, const cadf s) {
            lhs /= s;
            return lhs;
        }

        // ===== common methods =====

        /// @brief Normalizes in-place 
        /// 
        /// @pre Length must not be zero or near-zero
        void normalize() noexcept {
            const auto lengthSq = lengthSquared();
            assert(lengthSq > gc_eps * gc_eps && "normalize() called on a zero or near-zero vector");
            if (std::abs(lengthSq - static_cast<T>(1)) < gc_eps) {
                return;
            }

            *this /= std::sqrt(lengthSq);
        }

        /// @brief Returns a normalized copy.
        ///
        /// @pre Length must not be zero or near-zero
        [[nodiscard]] Quaternion normalized() const noexcept {
            Quaternion res = static_cast<const Quaternion&>(*this);
            res.normalize();
            return res;
        }

        /// @brief Returns a normalized copy, or `fallback` if the vector length is <= eps
        [[nodiscard]] Quaternion safeNormalized(const Quaternion &fallback) const noexcept {
            const auto lengthSq = lengthSquared();
            if (lengthSq < gc_eps * gc_eps) {
                return fallback;
            }
            return static_cast<const Quaternion&>(*this) / std::sqrt(lengthSq);
        }

        constexpr T length() const noexcept {
            T res = lengthSquared();
            return std::sqrt(res);
        }

        constexpr T lengthSquared() const noexcept {
            T res{};
            for (int i = 0; i < 4; ++i) {
                res += (*this)[i] * (*this)[i];
            }
            return res;
        }
    };
}

#endif //CAD_QUATERNION_HXX
