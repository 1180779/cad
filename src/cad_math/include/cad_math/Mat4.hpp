//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MAT4_H
#define CAD_MAT4_H

#include "MatBase.hpp"
#include "Vec4.hpp"

#include "Mat3.hpp"

namespace cadm {
    template <std::size_t R, std::size_t C, typename T>
    struct Mat;

    template <>
    struct Mat<4, 4, cadf> : MatBase<Mat, Mat<4, 4, cadf>, vec4, vec4, 4, 4, cadf> {
        union {
            cadf data[16]{};
            vec4 columns[4];
        };

        constexpr Mat() {
            for (auto &cell : data) {
                cell = 0;
            }
        }

        constexpr Mat(
            const cadf x0,
            const cadf x1,
            const cadf x2,
            const cadf x3,
            const cadf y0,
            const cadf y1,
            const cadf y2,
            const cadf y3,
            const cadf z0,
            const cadf z1,
            const cadf z2,
            const cadf z3,
            const cadf w0,
            const cadf w1,
            const cadf w2,
            const cadf w3
        ) {
            columns[0] = vec4(x0, x1, x2, x3);
            columns[1] = vec4(y0, y1, y2, y3);
            columns[2] = vec4(z0, z1, z2, z3);
            columns[3] = vec4(w0, w1, w2, w3);
        }

        constexpr Mat(const vec4 &c0, const vec4 &c1, const vec4 &c2, const vec4 &c3) {
            columns[0] = c0;
            columns[1] = c1;
            columns[2] = c2;
            columns[3] = c3;
        }

        constexpr static Mat identity() {
            return Mat{
                vec4::unitX(),
                vec4::unitY(),
                vec4::unitZ(),
                vec4::unitW()
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
                vec4(xAxis.x, yAxis.x, zAxis.x, 0.0),
                vec4(xAxis.y, yAxis.y, zAxis.y, 0.0),
                vec4(xAxis.z, yAxis.z, zAxis.z, 0.0),
                vec4(-xAxis.dot(eye), -yAxis.dot(eye), -zAxis.dot(eye), 1.0),
            };
        }

        constexpr static Mat ortho(
            const cadf left,
            const cadf right,
            const cadf bottom,
            const cadf top,
            const cadf near,
            const cadf far
        ) {
            return {
                vec4(static_cast<cadf>(2.0 / (right - left)), 0.0, 0.0, 0.0),
                vec4(0.0, static_cast<cadf>(2.0 / (top - bottom)), 0.0, 0.0),
                vec4(0.0, 0.0, static_cast<cadf>(-2.0 / (far - near)), 0.0),
                vec4(
                    -(right + left) / (right - left),
                    -(top + bottom) / (top - bottom),
                    -(far + near) / (far - near),
                    1.0
                )
            };
        }

        // TODO: further investigate
        // https://stackoverflow.com/questions/3738384/stable-cotangent

        // TODO: check if constexpr is allowed here in MSVC

        // projection minus one to one
        static Mat projectionMo(const cadf aspect, const cadf fov, const cadf near, const cadf far) {
            const cadf ctg = std::cos(fov / 2) / std::sin(fov / 2);
            return {
                vec4(ctg / aspect, 0, 0, 0),
                vec4(0, ctg, 0, 0),
                vec4(0, 0, -(far + near) / (far - near), -1),
                vec4(0, 0, -2 * far * near / (far - near), 0),
            };
        }

        /// @brief Off-axis (asymmetric) perspective frustum (the MPO matrix). NDC z in [-1, 1].
        /// Used to build per-eye projections for stereoscopy.
        static Mat frustum(
            const cadf left,
            const cadf right,
            const cadf bottom,
            const cadf top,
            const cadf near,
            const cadf far
        ) {
            return {
                vec4(2 * near / (right - left), 0, 0, 0),
                vec4(0, 2 * near / (top - bottom), 0, 0),
                vec4(
                    (right + left) / (right - left),
                    (top + bottom) / (top - bottom),
                    -(far + near) / (far - near),
                    -1
                ),
                vec4(0, 0, -2 * far * near / (far - near), 0),
            };
        }

        constexpr static Mat scale(const Vec3 &s) {
            return scale(s.x, s.y, s.z);
        }

        constexpr static Mat scale(const cadf sx, const cadf sy, const cadf sz) {
            return diag(sx, sy, sz, 1.0);
        }

        constexpr static Mat diag(const cadf d0, const cadf d1, const cadf d2, const cadf d3) {
            return Mat{
                {d0, 0, 0, 0},
                {0, d1, 0, 0},
                {0, 0, d2, 0},
                {0, 0, 0, d3}
            };
        }

        constexpr static Mat translation(const Vec3 &t) {
            return translation(t.x, t.y, t.z);
        }

        constexpr static Mat translation(const cadf tx, const cadf ty, const cadf tz) {
            return {
                vec4::unitX(),
                vec4::unitY(),
                vec4::unitZ(),
                vec4(tx, ty, tz, 1.0),
            };
        }

        static Mat rotX(const cadf alpha) {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                vec4::unitX(),
                vec4(0, c, s, 0),
                vec4(0, -s, c, 0),
                vec4::unitW()
            };
        }

        static Mat rotY(const cadf alpha) {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                vec4(c, 0, -s, 0),
                vec4::unitY(),
                vec4(s, 0, c, 0),
                vec4::unitW()
            };
        }

        static Mat rotZ(const cadf alpha) {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                vec4(c, s, 0, 0),
                vec4(-s, c, 0, 0),
                vec4::unitZ(),
                vec4::unitW()
            };
        }

        static Mat rotZyx(const Vec3 &xyz) {
            return rotZ(xyz.z) * rotY(xyz.y) * rotX(xyz.x);
        }

        static Mat rotZyx(const cadf x, const cadf y, const cadf z) {
            return rotZ(z) * rotY(y) * rotX(x);
        }

        /// @brief Rodrigues rotation matrix around axis `u` by angle `phi` (radians)
        ///
        /// @pre u must be a unit vector
        static Mat rotAxis(const cadf phi, const Vec3 &u) {
            assert(std::abs(u.lengthSquared() - cadf{1}) < gc_eps && "rotAxis: axis must be a unit vector");
            const auto sin = std::sin(phi);
            const auto cos = std::cos(phi);
            const auto oneMinusCos = 1 - cos;
            return {
                vec4{
                    u.x * u.x * oneMinusCos + cos,
                    u.x * u.y * oneMinusCos + u.z * sin,
                    u.x * u.z * oneMinusCos - u.y * sin,
                    0
                },
                vec4{
                    u.x * u.y * oneMinusCos - u.z * sin,
                    u.y * u.y * oneMinusCos + cos,
                    u.y * u.z * oneMinusCos + u.x * sin,
                    0
                },
                vec4{
                    u.x * u.z * oneMinusCos + u.y * sin,
                    u.y * u.z * oneMinusCos - u.x * sin,
                    u.z * u.z * oneMinusCos + cos,
                    0
                },
                vec4{0, 0, 0, 1}
            };
        }

        [[nodiscard]] constexpr Mat3 upperLeft3X3() const {
            return {
                Vec3(col(0)[0], col(0)[1], col(0)[2]),
                Vec3(col(1)[0], col(1)[1], col(1)[2]),
                Vec3(col(2)[0], col(2)[1], col(2)[2]),
            };
        }

        constexpr void inverseTranslation() {
            (*this)(0, 3) = -(*this)(0, 3);
            (*this)(1, 3) = -(*this)(1, 3);
            (*this)(2, 3) = -(*this)(2, 3);
        }

        [[nodiscard]] constexpr Mat inversedTranslation() const {
            auto copy = *this;
            copy.inverseTranslation();
            return copy;
        }

        constexpr void inverseScale() {
            if ((*this)(0, 0) != 0.0) {
                (*this)(0, 0) = static_cast<cadf>(1.0 / (*this)(0, 0));
            }
            if ((*this)(1, 1) != 0.0) {
                (*this)(1, 1) = static_cast<cadf>(1.0 / (*this)(1, 1));
            }
            if ((*this)(2, 2) != 0.0) {
                (*this)(2, 2) = static_cast<cadf>(1.0 / (*this)(2, 2));
            }
        }

        [[nodiscard]] constexpr Mat inversedScale() const {
            auto copy = *this;
            copy.inverseScale();
            return copy;
        }

        constexpr void inverseRotation() {
            transpose();
        }

        [[nodiscard]] constexpr Mat inversedRotation() const {
            return transposed();
        }

        void inverseView() {
            *this = inversedView();
        }

        [[nodiscard]] constexpr Mat inversedView() const {
            const auto col0 = col(0).xyz();
            const auto col1 = col(1).xyz();
            const auto col2 = col(2).xyz();
            const auto colt = col(3).xyz();

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

        void inverseProjectionMO() {
            *this = inversedProjectionMo();
        }

        [[nodiscard]] Mat inversedProjectionMo() const {
            const auto a = col(0)[0];
            const auto b = col(1)[1];
            const auto c = col(2)[2];
            const auto d = col(3)[2];

            return {
                {static_cast<cadf>(1.0 / a), 0, 0, 0},
                {0, static_cast<cadf>(1.0 / b), 0, 0},
                {0, 0, 0, static_cast<cadf>(1.0 / d)},
                {0, 0, -1, c / d},
            };
        }

        /// @brief Analytic inverse of an off-axis frustum() matrix (asymmetric perspective)
        [[nodiscard]] Mat inversedFrustum() const {
            const auto a = col(0)[0]; // 2n/(r-l)
            const auto b = col(1)[1]; // 2n/(t-b)
            const auto c = col(2)[0]; // (r+l)/(r-l)
            const auto d = col(2)[1]; // (t+b)/(t-b)
            const auto e = col(2)[2]; // -(f+n)/(f-n)
            const auto f = col(3)[2]; // -2fn/(f-n)

            return {
                {static_cast<cadf>(1.0 / a), 0, 0, 0},
                {0, static_cast<cadf>(1.0 / b), 0, 0},
                {0, 0, 0, static_cast<cadf>(1.0 / f)},
                {c / a, d / b, -1, e / f},
            };
        }

        void inverseOrtho() {
            *this = inversedOrtho();
        }

        [[nodiscard]] constexpr Mat inversedOrtho() const {
            const auto a = col(0)[0];
            const auto b = col(1)[1];
            const auto c = col(2)[2];
            const auto tx = col(3)[0];
            const auto ty = col(3)[1];
            const auto tz = col(3)[2];

            const auto ia = static_cast<cadf>(1.0 / a);
            const auto ib = static_cast<cadf>(1.0 / b);
            const auto ic = static_cast<cadf>(1.0 / c);

            return {
                {ia, 0, 0, 0},
                {0, ib, 0, 0},
                {0, 0, ic, 0},
                {-tx * ia, -ty * ib, -tz * ic, 1},
            };
        }
    };

    using Mat4 = Mat<4, 4, cadf>;
}

#endif //CAD_MAT4_H
