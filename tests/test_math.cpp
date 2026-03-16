//
// Created by rdkgsk on 3/1/26.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cad_math/vec3.h>
#include <cad_math/vec4.h>
#include <cad_math/mat4.h>

namespace
{
    using namespace cadm;

    TEST_CASE("vec3 basic operations", "[math][vec3]")
    {
        vec3 v1(1.0f, 2.0f, 3.0f);
        vec3 v2(4.0f, 5.0f, 6.0f);

        SECTION("Addition")
        {
            vec3 res = v1 + v2;
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(5.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(7.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(9.0f));
        }

        SECTION("Subtraction")
        {
            vec3 res = v2 - v1;
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(3.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(3.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(3.0f));
        }

        SECTION("Scalar Multiplication")
        {
            vec3 res = v1 * 2.0f;
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(2.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(4.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(6.0f));
        }

        SECTION("Dot Product")
        {
            float dot = v1.dot(v2);
            REQUIRE_THAT(dot, Catch::Matchers::WithinRel(32.0f));
        }

        SECTION("Cross Product")
        {
            vec3 res = v1.cross(v2);
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(-3.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(6.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(-3.0f));
        }
    }

    TEST_CASE("vec4 basic operations", "[math][vec4]")
    {
        vec4 v1(1.0f, 2.0f, 3.0f, 4.0f);
        vec4 v2(5.0f, 6.0f, 7.0f, 8.0f);

        SECTION("Addition")
        {
            vec4 res = v1 + v2;
            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(6.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(8.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(10.0f));
            REQUIRE_THAT(res.w, Catch::Matchers::WithinRel(12.0f));
        }
    }

    TEST_CASE("mat4 basic operations", "[math][mat4]")
    {
        mat4 m1 = mat4::identity();
        mat4 m2(
            vec4(1, 5, 9, 13),
            vec4(2, 6, 10, 14),
            vec4(3, 7, 11, 15),
            vec4(4, 8, 12, 16)
        );

        SECTION("Identity Multiplication")
        {
            mat4 res = m1 * m2;
            REQUIRE(res == m2);
        }

        SECTION("Matrix-Vector Multiplication")
        {
            vec4 v(1.0f, 1.0f, 1.0f, 1.0f);
            vec4 res = m2 * v;

            REQUIRE_THAT(res.x, Catch::Matchers::WithinRel(10.0f));
            REQUIRE_THAT(res.y, Catch::Matchers::WithinRel(26.0f));
            REQUIRE_THAT(res.z, Catch::Matchers::WithinRel(42.0f));
            REQUIRE_THAT(res.w, Catch::Matchers::WithinRel(58.0f));
        }
    }

    TEST_CASE("mat4 inverse", "[math][mat4]")
    {
        SECTION("Identity Inverse")
        {
            mat4 m = mat4::identity();
            mat4 inv = m.inversed();
            REQUIRE(inv == m);
        }

        SECTION("Simple Inverse")
        {
            mat4 m = mat4::diag(2.0, 0.5, 1.0, 1.0);

            mat4 inv = m.inversed();
            REQUIRE_THAT(inv(0, 0), Catch::Matchers::WithinRel(0.5f));
            REQUIRE_THAT(inv(1, 1), Catch::Matchers::WithinRel(2.0f));
            REQUIRE_THAT(inv(2, 2), Catch::Matchers::WithinRel(1.0f));
            REQUIRE_THAT(inv(3, 3), Catch::Matchers::WithinRel(1.0f));
        }

        SECTION("General Inverse")
        {
            mat4 m = mat4::identity();
            m(0, 0) = 1;
            m(0, 1) = 2;
            m(1, 0) = 3;
            m(1, 1) = 4;

            mat4 inv = m.inversed();

            // check A * A^-1 = I
            mat4 res = m * inv;
            REQUIRE(res == mat4::identity());
        }
    }

    TEST_CASE("mat4 inverseSafe", "[math][mat4]")
    {
        SECTION("Invertible Matrix")
        {
            mat4 m = mat4::identity();
            m(0, 0) = 1;
            m(0, 1) = 2;
            m(1, 0) = 3;
            m(1, 1) = 4;

            auto inv_opt = m.inversedSafe();
            REQUIRE(inv_opt.has_value());

            // check A * A^-1 = I
            mat4 res = m * inv_opt.value();
            REQUIRE(res == mat4::identity());
        }

        SECTION("Singular Matrix")
        {
            mat4 m = mat4::identity();
            m(0, 0) = 1;
            m(0, 1) = 2;
            m(1, 0) = 2;
            m(1, 1) = 4;

            auto inv_opt = m.inversedSafe();
            REQUIRE_FALSE(inv_opt.has_value());
        }

        SECTION("Zero Matrix")
        {
            mat4 m{};

            auto inv_opt = m.inversedSafe();
            REQUIRE_FALSE(inv_opt.has_value());
        }
    }

    TEST_CASE("mat_row_ref operator overloading", "[math][mat4][row_ref]")
    {
        SECTION("Compound assignment operators")
        {
            mat4 m = mat4::identity();

            // test operator+=
            auto row0 = m.makeRowRef(0);
            vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
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

        SECTION("Binary operators return vectors")
        {
            mat4 m = mat4::identity();
            auto row0 = m.makeRowRef(0);

            auto requireThatMatrixUnchanged = [&m]()
            {
                REQUIRE_THAT(m(0, 0), Catch::Matchers::WithinRel(1.0f));
                REQUIRE_THAT(m(0, 1), Catch::Matchers::WithinRel(0.0f));
                REQUIRE_THAT(m(0, 2), Catch::Matchers::WithinRel(0.0f));
                REQUIRE_THAT(m(0, 3), Catch::Matchers::WithinRel(0.0f));
            };

            vec4 v(1.0f, 1.0f, 1.0f, 1.0f);

            // operator+ returns vector, doesn't modify row
            vec4 result = row0 + v;
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

        SECTION("Assignment from vector")
        {
            mat4 m{};
            auto row1 = m.makeRowRef(1);

            vec4 v(5.0f, 6.0f, 7.0f, 8.0f);
            row1 = v;

            REQUIRE_THAT(m(1, 0), Catch::Matchers::WithinRel(5.0f));
            REQUIRE_THAT(m(1, 1), Catch::Matchers::WithinRel(6.0f));
            REQUIRE_THAT(m(1, 2), Catch::Matchers::WithinRel(7.0f));
            REQUIRE_THAT(m(1, 3), Catch::Matchers::WithinRel(8.0f));
        }

        SECTION("Implicit conversion to vector")
        {
            mat4 m = mat4::identity();
            m(2, 0) = 10.0f;
            m(2, 1) = 20.0f;
            m(2, 2) = 30.0f;
            m(2, 3) = 40.0f;

            auto row2 = m.makeRowRef(2);
            vec4 v = row2;

            REQUIRE_THAT(v.x, Catch::Matchers::WithinRel(10.0f));
            REQUIRE_THAT(v.y, Catch::Matchers::WithinRel(20.0f));
            REQUIRE_THAT(v.z, Catch::Matchers::WithinRel(30.0f));
            REQUIRE_THAT(v.w, Catch::Matchers::WithinRel(40.0f));
        }
    }
}
