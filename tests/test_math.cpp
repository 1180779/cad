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
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 14, 15, 16
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
}
