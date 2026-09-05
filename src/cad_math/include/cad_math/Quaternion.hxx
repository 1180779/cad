//
// Created by Radosław Głasek on 05.07.2026
//

#ifndef CAD_QUATERNION_HXX
#define CAD_QUATERNION_HXX

#include <algorithm>
#include <cmath>
#include <span>
#include <numbers>

#include "Mat3.hpp"
#include "VecBase.hpp"
#include "Vec4.hpp"

namespace cadm {
    template <typename T>
    class Quaternion final {
        union {
            T data[4]{};

            struct {
                T e0, e1, e2, e3;
            };
        };

    public:
        explicit Quaternion() {
            this->e0 = T{1};
            this->e1 = T{0};
            this->e2 = T{0};
            this->e3 = T{0};
        }

        explicit Quaternion(std::span<T, 4> d) {
            std::copy(d.begin(), d.end(), data);
        }

        explicit Quaternion(T e0, T e1, T e2, T e3) {
            this->e0 = e0;
            this->e1 = e1;
            this->e2 = e2;
            this->e3 = e3;
        }

        explicit Quaternion(T e0, const Vec<T, 3> &n) {
            this->e0 = e0;
            this->e1 = n[0];
            this->e2 = n[1];
            this->e3 = n[2];
        }

        constexpr T scalarPart() noexcept {
            return e0;
        }

        constexpr Vec<T, 3> vectorPart() noexcept {
            return {e1, e2, e3};
        }

        constexpr Vec<T, 3> vectorPart() const noexcept {
            return {e1, e2, e3};
        }

        constexpr void setVectorPart(const Vec<T, 3> &v) noexcept {
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
            return Quaternion{-v.e0, -v.e1, -v.e2, -v.e3};
        }

        Vec<T, 4> asVec() const noexcept {
            return {e0, e1, e2, e3};
        }

        // ===== quaternion-quaternion operators =====

        friend constexpr Vec<T, 3> rotateVec(const Quaternion &q, const Vec<T, 3> &v) {
            // const Quaternion qv{0, v};
            // const Quaternion conj = q.conjugate();
            // const Quaternion qRot = q * qv * conj;
            // return qRot.vectorPart();
            const T q0 = q.scalarPart();
            const Vec<T, 3> qv = q.vectorPart();
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

        constexpr static Quaternion fromRotation(T alpha, Vec<T, 3> n) {
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

        /// @brief Quaternion to ZYX Euler angles (x=roll, y=pitch, z=yaw)
        Vec<T, 3> toEuler() const noexcept {
            return Quaternion::toEuler(*this);
        }

        /// @brief Quaternion to ZYX Euler angles (x=roll, y=pitch, z=yaw)
        static Vec<T, 3> toEuler(const Quaternion &q) noexcept {
            const auto w = q[0],
                       x = q[1],
                       y = q[2],
                       z = q[3];

            const auto sinr = 2 * (w * x + y * z);
            const auto cosr = 1 - 2 * (x * x + y * y);
            const auto roll = std::atan2(sinr, cosr);

            const auto sinp = std::clamp(2 * (w * y - x * z), T{-1}, T{1});
            const auto pitch = -std::numbers::pi_v<cadf> / 2
                + 2 * std::atan2(std::sqrt(1 + sinp), std::sqrt(1 - sinp));

            const auto siny = 2 * (w * z + x * y);
            const auto cosy = 1 - 2 * (y * y + z * z);
            const auto yaw = std::atan2(siny, cosy);

            return {roll, pitch, yaw};
        }

        /// @brief Inverse of @ref Quaternion::toEuler
        static Quaternion fromEuler(const Vec<T, 3> &rotation) noexcept {
            const auto rotationHalf = rotation * 0.5f;
            const auto cr = std::cos(rotationHalf.x), sr = std::sin(rotationHalf.x);
            const auto cp = std::cos(rotationHalf.y), sp = std::sin(rotationHalf.y);
            const auto cy = std::cos(rotationHalf.z), sy = std::sin(rotationHalf.z);
            return Quaternion(
                cr * cp * cy + sr * sp * sy,
                sr * cp * cy - cr * sp * sy,
                cr * sp * cy + sr * cp * sy,
                cr * cp * sy - sr * sp * cy
            );
        }

        /// @brief Rotation matrix from a non-zero quaternion
        Mat<T, 3, 3> toRotationMatrix() const {
            // https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#From_a_quaternion_to_an_orthogonal_matrix
            const auto a = e0,
                       b = e1,
                       c = e2,
                       d = e3;
            const auto s = T{2} / (a * a + b * b + c * c + d * d);
            const auto bs = b * s,
                       cs = c * s,
                       ds = d * s;
            const auto ab = a * bs,
                       ac = a * cs,
                       ad = a * ds;
            const auto bb = b * bs,
                       bc = b * cs,
                       bd = b * ds;
            const auto cc = c * cs,
                       cd = c * ds,
                       dd = d * ds;
            return {
                Vec<T, 3>{1 - cc - dd, bc + ad, bd - ac},
                Vec<T, 3>{bc - ad, 1 - bb - dd, cd + ab},
                Vec<T, 3>{bd + ac, cd - ab, 1 - bb - cc}
            };
        }

        /// @brief Unit quaternion from a rotation matrix
        static Quaternion fromRotationMatrix(const Mat<T, 3, 3> &m) {
            // https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion
            const auto &x = m.row(0);
            const auto &y = m.row(1);
            const auto &z = m.row(2);
            if (const T trace = x.x + y.y + z.z;
                trace > T{gc_feps}) {
                const T r = std::sqrt(trace + 1);
                const T s = T{0.5} / r;
                return Quaternion{
                    T{0.5} * r,
                    (z.y - y.z) * s,
                    (x.z - z.x) * s,
                    (y.x - x.y) * s
                }.normalized();
            }
            if (x.x > y.y && x.x > z.z) {
                const T r = std::sqrt(1 + x.x - y.y - z.z);
                const T s = T{0.5} / r;
                return Quaternion{
                    (z.y - y.z) * s,
                    T{0.5} * r,
                    (x.y + y.x) * s,
                    (z.x + x.z) * s
                }.normalized();
            }
            if (y.y > z.z) {
                const T r = std::sqrt(1 + y.y - x.x - z.z);
                const T s = T{0.5} / r;
                return Quaternion{
                    (x.z - z.x) * s,
                    (x.y + y.x) * s,
                    T{0.5} * r,
                    (y.z + z.y) * s
                }.normalized();
            }
            const T r = std::sqrt(1 + z.z - x.x - y.y);
            const T s = T{0.5} / r;
            return Quaternion{
                (y.x - x.y) * s,
                (z.x + x.z) * s,
                (y.z + z.y) * s,
                T{0.5} * r
            }.normalized();
        }

        /// @brief Spherical linear interpolation between unit quaternions along the
        /// shorter arc; @p t in [0, 1]
        static Quaternion slerp(const Quaternion &p0, Quaternion p1, const T t) {
            // https://en.wikipedia.org/wiki/Spherical_linear_interpolation
            // theta = angle subtended by the arc
            T dot = p0.asVec().dot(p1.asVec());
            if (dot < T{0}) {
                // take the shorter of the two arcs
                p1 = -p1;
                dot = -dot;
            }
            if (dot > T{0.9995}) {
                // theta close to 0 => sin close to 0
                return (p0 + (p1 - p0) * t).normalized();
            }
            const T theta = std::acos(std::clamp(dot, T{-1}, T{1}));
            const T s = std::sin(theta);
            return (p0 * (std::sin((1 - t) * theta) / s) + p1 * (std::sin(t * theta) / s)).normalized();
        }
    };

    using Quat = Quaternion<cadf>;
}

#endif //CAD_QUATERNION_HXX
