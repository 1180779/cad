//
// Created by rdkgsk on 3/1/26.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cad_math/Vec3.hpp>
#include <cad_math/Vec4.hpp>
#include <cad_math/Mat4.hpp>
#include <cad_math/Helpers.hpp>
#include <numbers>

namespace {
    using namespace cadm;

    constexpr cadf kPi = std::numbers::pi_v<cadf>;

    TEST_CASE(
        "eulerZYXFromRotMat",
        "[math][euler]"
    ) {
        SECTION("Identity") {
            const auto m = Mat3::identity();
            const auto angles = eulerZYXFromRotMat(m);
            const auto rot = Mat3::rotZyx(angles);
            REQUIRE(rot == m);
        }

        SECTION("Pure Rx rotation") {
            const auto m = Mat3::rotX(kPi / 4.0f); // 45 deg
            const auto angles = eulerZYXFromRotMat(m);
            const auto rot = Mat3::rotZyx(angles);
            REQUIRE(rot == m);
        }

        SECTION("Pure Ry rotation") {
            const auto m = Mat3::rotY(kPi / 6.0f); // 30 deg
            const auto angles = eulerZYXFromRotMat(m);
            const auto rot = Mat3::rotZyx(angles);
            REQUIRE(rot == m);
        }

        SECTION("Combined ZYX rotation") {
            const auto m = Mat3::rotZyx(kPi / 6.0f, kPi / 4.0f, kPi / 3.0f); // 30, 45, 60 deg
            const auto angles = eulerZYXFromRotMat(m);
            const auto rot = Mat3::rotZyx(angles);
            REQUIRE(rot == m);
        }

        SECTION("Gimbal lock ry = pi/2") {
            const auto m = Mat3::rotZyx(kPi / 5.0f, kPi / 2.0f, 0.0f);
            const auto angles = eulerZYXFromRotMat(m);
            const auto rot = Mat3::rotZyx(angles.x, angles.y, angles.z);
            REQUIRE(rot == m);
        }
    }

    TEST_CASE(
        "vec3 basic operations",
        "[math][vec3]"
    ) {
        Vec3 v1(1.0f, 2.0f, 3.0f);
        Vec3 v2(4.0f, 5.0f, 6.0f);

        SECTION("Addition") {
            Vec3 res = v1 + v2;
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(5.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(7.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(9.0f));
        }

        SECTION("Subtraction") {
            Vec3 res = v2 - v1;
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(3.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(3.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(3.0f));
        }

        SECTION("Scalar Multiplication") {
            Vec3 res = v1 * 2.0f;
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(2.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(4.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(6.0f));
        }

        SECTION("Dot Product") {
            float dot = v1.dot(v2);
            REQUIRE_THAT(dot, Catch::Matchers::WithinRel(32.0f));
        }

        SECTION("Cross Product") {
            Vec3 res = v1.cross(v2);
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(-3.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(6.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(-3.0f));
        }
    }

    TEST_CASE(
        "vec4 basic operations",
        "[math][vec4]"
    ) {
        Vec4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        Vec4 v2(5.0f, 6.0f, 7.0f, 8.0f);

        SECTION("Addition") {
            Vec4 res = v1 + v2;
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(6.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(8.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(10.0f));
            REQUIRE_THAT(res.w, Catch::Matchers::WithinRel(12.0f));
        }
    }

    TEST_CASE(
        "mat4 basic operations",
        "[math][mat4]"
    ) {
        Mat4 m1 = Mat4::identity();
        Mat4 m2(
            Vec4(1, 5, 9, 13),
            Vec4(2, 6, 10, 14),
            Vec4(3, 7, 11, 15),
            Vec4(4, 8, 12, 16)
        );

        SECTION("Identity Multiplication") {
            Mat4 res = m1 * m2;
            REQUIRE(res == m2);
        }

        SECTION("Matrix-Vector Multiplication") {
            Vec4 v(1.0f, 1.0f, 1.0f, 1.0f);
            Vec4 res = m2 * v;

            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(10.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(26.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(42.0f));
            REQUIRE_THAT(res.w, Catch::Matchers::WithinRel(58.0f));
        }
    }

    TEST_CASE(
        "mat4 inverse",
        "[math][mat4]"
    ) {
        SECTION("Identity Inverse") {
            Mat4 m = Mat4::identity();
            Mat4 inv = m.inversed();
            REQUIRE(inv == m);
        }

        SECTION("Simple Inverse") {
            Mat4 m = Mat4::diag(2.0, 0.5, 1.0, 1.0);

            Mat4 inv = m.inversed();
            REQUIRE_THAT(inv(0, 0), Catch::Matchers::WithinRel(0.5f));
            REQUIRE_THAT(inv(1, 1), Catch::Matchers::WithinRel(2.0f));
            REQUIRE_THAT(inv(2, 2), Catch::Matchers::WithinRel(1.0f));
            REQUIRE_THAT(inv(3, 3), Catch::Matchers::WithinRel(1.0f));
        }

        SECTION("General Inverse") {
            Mat4 m = Mat4::identity();
            m(0, 0) = 1;
            m(0, 1) = 2;
            m(1, 0) = 3;
            m(1, 1) = 4;

            Mat4 inv = m.inversed();

            // check A * A^-1 = I
            Mat4 res = m * inv;
            REQUIRE(res == Mat4::identity());
        }
    }

    TEST_CASE(
        "mat4 inverseSafe",
        "[math][mat4]"
    ) {
        SECTION("Invertible Matrix") {
            Mat4 m = Mat4::identity();
            m(0, 0) = 1;
            m(0, 1) = 2;
            m(1, 0) = 3;
            m(1, 1) = 4;

            auto inv_opt = m.inversedSafe();
            REQUIRE(inv_opt.has_value());

            // check A * A^-1 = I
            Mat4 res = m * inv_opt.value();
            REQUIRE(res == Mat4::identity());
        }

        SECTION("Singular Matrix") {
            Mat4 m = Mat4::identity();
            m(0, 0) = 1;
            m(0, 1) = 2;
            m(1, 0) = 2;
            m(1, 1) = 4;

            auto inv_opt = m.inversedSafe();
            REQUIRE_FALSE(inv_opt.has_value());
        }

        SECTION("Zero Matrix") {
            Mat4 m{};

            auto inv_opt = m.inversedSafe();
            REQUIRE_FALSE(inv_opt.has_value());
        }
    }

    TEST_CASE(
        "mat_row_ref operator overloading",
        "[math][mat4][row_ref]"
    ) {
        SECTION("Compound assignment operators") {
            Mat4 m = Mat4::identity();

            // test operator+=
            auto row0 = m.makeRowRef(0);
            Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
            row0 += v;
            REQUIRE_THAT(m(0, 0), Catch::Matchers::WithinRel(2.0f));
            REQUIRE_THAT(m(0, 1), Catch::Matchers::WithinRel(2.0f));
            REQUIRE_THAT(m(0, 2), Catch::Matchers::WithinRel(3.0f));
            REQUIRE_THAT(m(0, 3), Catch::Matchers::WithinRel(4.0f));

            // test operator-=
            row0 -= v;
            REQUIRE_THAT(m(0, 0), Catch::Matchers::WithinRel(1.0f));
            REQUIRE_THAT(m(0, 1), Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(m(0, 2), Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(m(0, 3), Catch::Matchers::WithinRel(0.0f));

            // test operator*=
            row0 *= 2.0f;
            REQUIRE_THAT(m(0, 0), Catch::Matchers::WithinRel(2.0f));
            REQUIRE_THAT(m(0, 1), Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(m(0, 2), Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(m(0, 3), Catch::Matchers::WithinRel(0.0f));

            // test operator/=
            row0 /= 2.0f;
            REQUIRE_THAT(m(0, 0), Catch::Matchers::WithinRel(1.0f));
            REQUIRE_THAT(m(0, 1), Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(m(0, 2), Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(m(0, 3), Catch::Matchers::WithinRel(0.0f));
        }

        SECTION("Binary operators return vectors") {
            Mat4 m = Mat4::identity();
            auto row0 = m.makeRowRef(0);

            auto requireThatMatrixUnchanged = [&m]() {
                REQUIRE_THAT(m(0, 0), Catch::Matchers::WithinRel(1.0f));
                REQUIRE_THAT(m(0, 1), Catch::Matchers::WithinRel(0.0f));
                REQUIRE_THAT(m(0, 2), Catch::Matchers::WithinRel(0.0f));
                REQUIRE_THAT(m(0, 3), Catch::Matchers::WithinRel(0.0f));
            };

            Vec4 v(1.0f, 1.0f, 1.0f, 1.0f);

            // operator+ returns vector, doesn't modify row
            Vec4 result = row0 + v;
            REQUIRE_THAT(result.x, Catch::Matchers::WithinRel(2.0f));
            REQUIRE_THAT(result.y, Catch::Matchers::WithinRel(1.0f));
            REQUIRE_THAT(result.z, Catch::Matchers::WithinRel(1.0f));
            REQUIRE_THAT(result.w, Catch::Matchers::WithinRel(1.0f));
            requireThatMatrixUnchanged();

            // operator- returns vector
            result = row0 - v;
            REQUIRE_THAT(result.x, Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(result.y, Catch::Matchers::WithinRel(-1.0f));
            REQUIRE_THAT(result.z, Catch::Matchers::WithinRel(-1.0f));
            REQUIRE_THAT(result.w, Catch::Matchers::WithinRel(-1.0f));
            requireThatMatrixUnchanged();

            // operator* returns vector
            result = row0 * 3.0f;
            REQUIRE_THAT(result.x, Catch::Matchers::WithinRel(3.0f));
            REQUIRE_THAT(result.y, Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(result.z, Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(result.w, Catch::Matchers::WithinRel(0.0f));
            requireThatMatrixUnchanged();

            // operator/ returns vector
            result = row0 / 2.0f;
            REQUIRE_THAT(result.x, Catch::Matchers::WithinRel(0.5f));
            REQUIRE_THAT(result.y, Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(result.z, Catch::Matchers::WithinRel(0.0f));
            REQUIRE_THAT(result.w, Catch::Matchers::WithinRel(0.0f));
            requireThatMatrixUnchanged();
        }

        SECTION("Assignment from vector") {
            Mat4 m{};
            // ReSharper disable once CppDFAUnusedValue
            // ReSharper disable once CppDFAUnreadVariable

            auto row1 = m.makeRowRef(1);

            Vec4 v(5.0f, 6.0f, 7.0f, 8.0f);
            // ReSharper disable once CppDFAUnusedValue
            row1 = v;

            REQUIRE_THAT(m(1, 0), Catch::Matchers::WithinRel(5.0f));
            REQUIRE_THAT(m(1, 1), Catch::Matchers::WithinRel(6.0f));
            REQUIRE_THAT(m(1, 2), Catch::Matchers::WithinRel(7.0f));
            REQUIRE_THAT(m(1, 3), Catch::Matchers::WithinRel(8.0f));
        }

        SECTION("Implicit conversion to vector") {
            Mat4 m = Mat4::identity();
            m(2, 0) = 10.0f;
            m(2, 1) = 20.0f;
            m(2, 2) = 30.0f;
            m(2, 3) = 40.0f;

            auto row2 = m.makeRowRef(2);
            auto v = static_cast<Vec4>(row2);

            REQUIRE_THAT(v.x, Catch::Matchers::WithinRel(10.0f));
            REQUIRE_THAT(v.y, Catch::Matchers::WithinRel(20.0f));
            REQUIRE_THAT(v.z, Catch::Matchers::WithinRel(30.0f));
            REQUIRE_THAT(v.w, Catch::Matchers::WithinRel(40.0f));
        }
    }
}
