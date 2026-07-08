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

    template <>
    struct Mat<cadf, 3, 3> : MatBase<cadf, 3, 3, Vec3, Vec3, Mat, Mat<cadf, 3, 3>> {
        union {
            cadf data[9]{};
            Vec3 columns[3];
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
            const cadf y0,
            const cadf y1,
            const cadf y2,
            const cadf z0,
            const cadf z1,
            const cadf z2
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
                1,
                0,
                0,
                0,
                1,
                0,
                0,
                0,
                1,
            };
        }

        constexpr static Mat scale(const Vec2 &s) {
            return scale(s.x, s.y);
        }

        constexpr static Mat scale(const cadf sx, const cadf sy) {
            return diag(sx, sy, 1.0);
        }

        constexpr static Mat diag(const cadf m0, const cadf m1, const cadf m2) {
            return {
                m0,
                0,
                0,
                0,
                m1,
                0,
                0,
                0,
                m2,
            };
        }

        constexpr static Mat translation(const Vec2 &t) {
            return translation(t.x, t.y);
        }

        constexpr static Mat translation(const cadf tx, const cadf ty) {
            return {
                1,
                0,
                0,
                0,
                1,
                0,
                tx,
                ty,
                1
            };
        }

        static Mat rotX(const cadf alpha) {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                1,
                0,
                0,
                0,
                c,
                s,
                0,
                -s,
                c,
            };
        }

        static Mat rotY(const cadf alpha) {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                c,
                0,
                -s,
                0,
                1,
                0,
                s,
                0,
                c,
            };
        }

        static Mat rotZyx(const Vec3 &xyz) {
            return rotZ(xyz.z) * rotY(xyz.y) * rotX(xyz.x);
        }

        static Mat rotZyx(const cadf rx, const cadf ry, const cadf rz) {
            return rotZ(rz) * rotY(ry) * rotX(rx);
        }

        static Mat rotZ(const cadf alpha) {
            const cadf c = std::cos(alpha);
            const cadf s = std::sin(alpha);

            return {
                c,
                s,
                0,
                -s,
                c,
                0,
                0,
                0,
                1,
            };
        }
    };

    using Mat3 = Mat<cadf, 3, 3>;
}
#endif //CAD_MAT3_H
