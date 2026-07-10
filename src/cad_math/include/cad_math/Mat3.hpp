//
// Created on 3/7/26.
//

#ifndef CAD_MAT3_H
#define CAD_MAT3_H
#include "Common.hpp"
#include "MatBase.hpp"
#include "Vec2.hpp"
#include "Vec3.hpp"

namespace cadm {
    template <typename T, std::size_t R, std::size_t C>
    struct Mat;

    template <typename T>
    struct Mat<T, 3, 3> : MatBase<T, 3, 3, Vec3, Vec3, Mat, Mat<T, 3, 3>> {
        union {
            T data[9]{};
            Vec<T, 3> columns[3];
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
            const T y0,
            const T y1,
            const T y2,
            const T z0,
            const T z1,
            const T z2
        ) {
            columns[0] = Vec3(x0, x1, x2);
            columns[1] = Vec3(y0, y1, y2);
            columns[2] = Vec3(z0, z1, z2);
        }

        constexpr Mat(const Vec3 &c0, const Vec3 &c1, const Vec3 &c2) {
            columns[0] = c0;
            columns[1] = c1;
            columns[2] = c2;
        }

        constexpr static Mat identity() {
            return {
                T{1},
                T{0},
                T{0},
                T{0},
                T{1},
                T{0},
                T{0},
                T{0},
                T{1},
            };
        }

        constexpr static Mat scale(const Vec<T, 2> &s) {
            return scale(s.x, s.y);
        }

        constexpr static Mat scale(const T sx, const T sy) {
            return diag(sx, sy, 1.0);
        }

        constexpr static Mat diag(const T m0, const T m1, const T m2) {
            return {
                T{m0},
                T{0},
                T{0},
                T{0},
                T{m1},
                T{0},
                T{0},
                T{0},
                T{m2},
            };
        }

        constexpr static Mat translation(const Vec2 &t) {
            return translation(t.x, t.y);
        }

        constexpr static Mat translation(const T tx, const T ty) {
            return {
                T{1},
                T{0},
                T{0},
                T{0},
                T{1},
                T{0},
                T{tx},
                T{ty},
                T{1}
            };
        }

        static Mat rotX(const T alpha) {
            const T c = std::cos(alpha);
            const T s = std::sin(alpha);

            return {
                T{1},
                T{0},
                T{0},
                T{0},
                T{c},
                T{s},
                T{0},
                T{-s},
                T{c},
            };
        }

        static Mat rotY(const T alpha) {
            const T c = std::cos(alpha);
            const T s = std::sin(alpha);

            return {
                T{c},
                T{0},
                T{-s},
                T{0},
                T{1},
                T{0},
                T{s},
                T{0},
                T{c},
            };
        }

        static Mat rotZyx(const Vec3 &xyz) {
            return rotZ(xyz.z) * rotY(xyz.y) * rotX(xyz.x);
        }

        static Mat rotZyx(const T rx, const T ry, const T rz) {
            return rotZ(rz) * rotY(ry) * rotX(rx);
        }

        static Mat rotZ(const T alpha) {
            const T c = std::cos(alpha);
            const T s = std::sin(alpha);

            return {
                T{c},
                T{s},
                T{0},
                T{-s},
                T{c},
                T{0},
                T{0},
                T{0},
                T{1},
            };
        }
    };

    using Mat3 = Mat<cadf, 3, 3>;
}
#endif //CAD_MAT3_H
