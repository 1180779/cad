//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MAT4_H
#define CAD_MAT4_H

#include "MatBase.hpp"
#include "Vec4.hpp"

#include "Mat3.hpp"

namespace cadm {
    template <typename T, std::size_t R, std::size_t C>
    struct Mat;

    template <typename T>
    struct Mat<T, 4, 4> : MatBase<T, 4, 4, Vec<T, 4>, Vec<T, 4>, Mat, Mat<T, 4, 4>> {
        union {
            T data[16]{};
            Vec<T, 4> columns[4];
        };

        constexpr Mat() {
            for (auto &cell : data) {
                cell = 0;
            }
        }

        constexpr Mat(
            const T x0,
            const T x1,
            const T x2,
            const T x3,
            const T y0,
            const T y1,
            const T y2,
            const T y3,
            const T z0,
            const T z1,
            const T z2,
            const T z3,
            const T w0,
            const T w1,
            const T w2,
            const T w3
        ) {
            columns[0] = Vec4(x0, x1, x2, x3);
            columns[1] = Vec4(y0, y1, y2, y3);
            columns[2] = Vec4(z0, z1, z2, z3);
            columns[3] = Vec4(w0, w1, w2, w3);
        }

        constexpr Mat(const Vec4 &c0, const Vec4 &c1, const Vec4 &c2, const Vec4 &c3) {
            columns[0] = c0;
            columns[1] = c1;
            columns[2] = c2;
            columns[3] = c3;
        }

        constexpr static Mat identity() {
            return Mat{
                Vec4::unitX(),
                Vec4::unitY(),
                Vec4::unitZ(),
                Vec4::unitW()
            };
        }

        // useful article to revisit view matrix:
        // https://www.3dgep.com/understanding-the-view-matrix/

        /// @brief Look at matrix for Right-Handed coordinate system
        constexpr static Mat lookAtRh(const Vec3 &eye, const Vec3 &target, const Vec3 &up) {
            const Vec3 zAxis = (eye - target).normalized(); // backward (away from target)
            const Vec3 xAxis = up.cross(zAxis).normalized(); // right
            const Vec3 yAxis = zAxis.cross(xAxis); // up

            return {
                Vec4(xAxis.x, yAxis.x, zAxis.x, T{0}),
                Vec4(xAxis.y, yAxis.y, zAxis.y, T{0}),
                Vec4(xAxis.z, yAxis.z, zAxis.z, T{0}),
                Vec4(-xAxis.dot(eye), -yAxis.dot(eye), -zAxis.dot(eye), 1.0),
            };
        }

        constexpr static Mat ortho(
            const T left,
            const T right,
            const T bottom,
            const T top,
            const T near,
            const T far
        ) {
            return {
                Vec4(static_cast<T>(T{2} / (right - left)), T{0}, T{0}, T{0}),
                Vec4(T{0}, static_cast<T>(T{2} / (top - bottom)), T{0}, T{0}),
                Vec4(T{0}, T{0}, static_cast<T>(T{-2} / (far - near)), T{0}),
                Vec4(
                    -(right + left) / (right - left),
                    -(top + bottom) / (top - bottom),
                    -(far + near) / (far - near),
                    T{1}
                )
            };
        }

        // TODO: further investigate
        // https://stackoverflow.com/questions/3738384/stable-cotangent

        // TODO: check if constexpr is allowed here in MSVC

        /// @brief Symmetric perspective frustum. NDC z in [-1, 1]
        static Mat perspective(const T aspect, const T fov, const T near, const T far) {
            const T ctg = std::cos(fov / T{2}) / std::sin(fov / T{2});
            return {
                Vec4(ctg / aspect, T{0}, T{0}, T{0}),
                Vec4(T{0}, ctg, T{0}, T{0}),
                Vec4(T{0}, T{0}, -(far + near) / (far - near), T{-1}),
                Vec4(T{0}, T{0}, T{-2} * far * near / (far - near), T{0}),
            };
        }

        /// @brief Off-axis (asymmetric) perspective frustum. NDC z in [-1, 1]
        static Mat frustum(
            const T left,
            const T right,
            const T bottom,
            const T top,
            const T near,
            const T far
        ) {
            return {
                Vec<T, 4>(T{2} * near / (right - left), T{0}, T{0}, T{0}),
                Vec<T, 4>(T{0}, T{2} * near / (top - bottom), T{0}, T{0}),
                Vec<T, 4>(
                    (right + left) / (right - left),
                    (top + bottom) / (top - bottom),
                    -(far + near) / (far - near),
                    T{-1}
                ),
                Vec<T, 4>(T{0}, T{0}, T{-2} * far * near / (far - near), T{0}),
            };
        }

        /// @brief Frustum edges recovered from a perspective matrix
        struct Frustum {
            T left, right, bottom, top, near, far;
        };

        /// @brief True if this is a perspective projection, not orthographic
        [[nodiscard]] bool isPerspective() const {
            return std::abs(this->col(2)[3] + static_cast<T>(1)) < gc_eps;
        }

        /// @brief Decomposes a frustum()/perspective() matrix back into its frustum edges
        [[nodiscard]] Frustum toFrustum() const {
            const auto a = this->col(0)[0]; // 2n/(r - l)
            const auto b = this->col(1)[1]; // 2n/(t - b)
            const auto c = this->col(2)[0]; // (r + l)/(r - l)
            const auto d = this->col(2)[1]; // (t + b)/(t - b)
            const auto e = this->col(2)[2]; // -(f + n)/(f - n)
            const auto g = this->col(3)[2]; // -2fn/(f - n)
            // e - 1 = -(f + n - f + n)/(f - n) = -2n/(f - n)
            // e + 1 = -(f + n + f - n)/(f - n) = -2f/(f - n)
            // c - 1 = (r + l - r + l)/(r - l) = 2l/(r - l)
            // c + 1 = (r + l + r - l)/(r - l) = 2r/(r - l)
            // d - 1 = (t + b - t + b)/(t - b) = 2b/(t - b)
            // d + 1 = (t + b + t - b)/(t - b) = 2t/(t - b)

            const T near = g / (e - 1);
            const T far = g / (e + 1);
            return {
                .left = near * (c - 1) / a,
                .right = near * (c + 1) / a,
                .bottom = near * (d - 1) / b,
                .top = near * (d + 1) / b,
                .near = near,
                .far = far,
            };
        }

        constexpr static Mat scale(const Vec3 &s) {
            return scale(s.x, s.y, s.z);
        }

        constexpr static Mat scale(const T sx, const T sy, const T sz) {
            return diag(sx, sy, sz, 1.0);
        }

        constexpr static Mat diag(const T d0, const T d1, const T d2, const T d3) {
            return Mat{
                {d0, T{0}, T{0}, T{0}},
                {T{0}, d1, T{0}, T{0}},
                {T{0}, T{0}, d2, T{0}},
                {T{0}, T{0}, T{0}, d3}
            };
        }

        constexpr static Mat translation(const Vec3 &t) {
            return translation(t.x, t.y, t.z);
        }

        constexpr static Mat translation(const T tx, const T ty, const T tz) {
            return {
                Vec<T, 4>::unitX(),
                Vec<T, 4>::unitY(),
                Vec<T, 4>::unitZ(),
                Vec<T, 4>(tx, ty, tz, T{1}),
            };
        }

        static Mat rotX(const T alpha) {
            const T c = std::cos(alpha);
            const T s = std::sin(alpha);

            return {
                Vec4::unitX(),
                Vec4(T{0}, c, s, T{0}),
                Vec4(T{0}, -s, c, T{0}),
                Vec4::unitW()
            };
        }

        static Mat rotY(const T alpha) {
            const T c = std::cos(alpha);
            const T s = std::sin(alpha);

            return {
                Vec4(c, T{0}, -s, T{0}),
                Vec4::unitY(),
                Vec4(s, T{0}, c, T{0}),
                Vec4::unitW()
            };
        }

        static Mat rotZ(const T alpha) {
            const T c = std::cos(alpha);
            const T s = std::sin(alpha);

            return {
                Vec4(c, s, T{0}, T{0}),
                Vec4(-s, c, T{0}, T{0}),
                Vec4::unitZ(),
                Vec4::unitW()
            };
        }

        static Mat rotZyx(const Vec<T, 3> &xyz) {
            return rotZ(xyz.z) * rotY(xyz.y) * rotX(xyz.x);
        }

        static Mat rotZyx(const T x, const T y, const T z) {
            return rotZ(z) * rotY(y) * rotX(x);
        }

        /// @brief Rodrigues rotation matrix around axis @p u by angle @p phi (in radians)
        ///
        /// @pre @p u must be a unit vector
        static Mat rotAxis(const T phi, const Vec<T, 3> &u) {
            assert(std::abs(u.lengthSquared() - T{1}) < gc_eps && "rotAxis: axis must be a unit vector");
            const auto sin = std::sin(phi);
            const auto cos = std::cos(phi);
            const auto oneMinusCos = 1 - cos;
            return {
                Vec<T, 4>{
                    u.x * u.x * oneMinusCos + cos,
                    u.x * u.y * oneMinusCos + u.z * sin,
                    u.x * u.z * oneMinusCos - u.y * sin,
                    0
                },
                Vec4{
                    u.x * u.y * oneMinusCos - u.z * sin,
                    u.y * u.y * oneMinusCos + cos,
                    u.y * u.z * oneMinusCos + u.x * sin,
                    0
                },
                Vec4{
                    u.x * u.z * oneMinusCos + u.y * sin,
                    u.y * u.z * oneMinusCos - u.x * sin,
                    u.z * u.z * oneMinusCos + cos,
                    0
                },
                Vec4{0, 0, 0, 1}
            };
        }

        [[nodiscard]] constexpr Mat<T, 3, 3> upperLeft3X3() const {
            return {
                Vec3(this->col(0)[0], this->col(0)[1], this->col(0)[2]),
                Vec3(this->col(1)[0], this->col(1)[1], this->col(1)[2]),
                Vec3(this->col(2)[0], this->col(2)[1], this->col(2)[2]),
            };
        }

        /// @brief Analytic inverse of translation matrix
        constexpr void inverseTranslation() {
            (*this)(0, 3) = -(*this)(0, 3);
            (*this)(1, 3) = -(*this)(1, 3);
            (*this)(2, 3) = -(*this)(2, 3);
        }

        /// @brief Analytic inverse of translation matrix
        [[nodiscard]] constexpr Mat inversedTranslation() const {
            auto copy = *this;
            copy.inverseTranslation();
            return copy;
        }

        /// @brief Analytic inverse of scale matrix
        constexpr void inverseScale() {
            if ((*this)(0, 0) != 0.0) {
                (*this)(0, 0) = static_cast<T>(1.0 / (*this)(0, 0));
            }
            if ((*this)(1, 1) != 0.0) {
                (*this)(1, 1) = static_cast<T>(1.0 / (*this)(1, 1));
            }
            if ((*this)(2, 2) != 0.0) {
                (*this)(2, 2) = static_cast<T>(1.0 / (*this)(2, 2));
            }
        }

        /// @brief Analytic inverse of scale matrix
        [[nodiscard]] constexpr Mat inversedScale() const {
            auto copy = *this;
            copy.inverseScale();
            return copy;
        }

        /// @brief Analytic inverse of rotation matrix
        constexpr void inverseRotation() {
            this->transpose();
        }

        /// @brief Analytic inverse of rotation matrix
        [[nodiscard]] constexpr Mat inversedRotation() const {
            return this->transposed();
        }

        /// @brief Analytic inverse of view matrix
        void inverseView() {
            *this = inversedView();
        }

        /// @brief Analytic inverse of view matrix
        [[nodiscard]] constexpr Mat inversedView() const {
            const auto col0 = this->col(0).xyz();
            const auto col1 = this->col(1).xyz();
            const auto col2 = this->col(2).xyz();
            const auto colt = this->col(3).xyz();

            const auto ntx = -col0.dot(colt);
            const auto nty = -col1.dot(colt);
            const auto ntz = -col2.dot(colt);

            return {
                {col0.x, col1.x, col2.x, 0},
                {col0.y, col1.y, col2.y, 0},
                {col0.z, col1.z, col2.z, 0},
                {ntx, nty, ntz, 1},
            };
        }

        /// @brief Analytic inverse of a symmetric perspective matrix
        void inversePerspective() {
            *this = inversedPerspective();
        }

        /// @brief Analytic inverse of a symmetric perspective matrix
        [[nodiscard]] Mat inversedPerspective() const {
            const auto a = this->col(0)[0];
            const auto b = this->col(1)[1];
            const auto c = this->col(2)[2];
            const auto d = this->col(3)[2];

            return {
                {static_cast<T>(T{1} / a), T{0}, T{0}, T{0}},
                {T{0}, static_cast<T>(T{1} / b), T{0}, T{0}},
                {T{0}, T{0}, T{0}, static_cast<T>(T{1} / d)},
                {T{0}, T{0}, T{-1}, c / d},
            };
        }

        /// @brief Analytic inverse of an asymmetric perspective matrix
        void inverseFrustum() {
            *this = inversedFrustum();
        }

        /// @brief Analytic inverse of an asymmetric perspective matrix
        [[nodiscard]] Mat inversedFrustum() const {
            const auto a = this->col(0)[0]; // 2n/(r-l)
            const auto b = this->col(1)[1]; // 2n/(t-b)
            const auto c = this->col(2)[0]; // (r+l)/(r-l)
            const auto d = this->col(2)[1]; // (t+b)/(t-b)
            const auto e = this->col(2)[2]; // -(f+n)/(f-n)
            const auto f = this->col(3)[2]; // -2fn/(f-n)

            return {
                {static_cast<T>(T{1} / a), T{0}, T{0}, T{0}},
                {T{0}, static_cast<T>(T{1} / b), T{0}, T{0}},
                {T{0}, T{0}, T{0}, static_cast<T>(T{1} / f)},
                {c / a, d / b, T{-1}, e / f},
            };
        }

        /// @brief Analytic inverse of orthogonal projection matrix
        void inverseOrtho() {
            *this = inversedOrtho();
        }

        /// @brief Analytic inverse of orthogonal projection matrix
        [[nodiscard]] constexpr Mat inversedOrtho() const {
            const auto a = this->col(0)[0];
            const auto b = this->col(1)[1];
            const auto c = this->col(2)[2];
            const auto tx = this->col(3)[0];
            const auto ty = this->col(3)[1];
            const auto tz = this->col(3)[2];

            const auto ia = static_cast<T>(1.0 / a);
            const auto ib = static_cast<T>(1.0 / b);
            const auto ic = static_cast<T>(1.0 / c);

            return {
                {ia, T{0}, T{0}, T{0}},
                {T{0}, ib, T{0}, T{0}},
                {T{0}, T{0}, ic, T{0}},
                {-tx * ia, -ty * ib, -tz * ic, T{1}},
            };
        }
    };

    using Mat4 = Mat<cadf, 4, 4>;
}

#endif //CAD_MAT4_H
