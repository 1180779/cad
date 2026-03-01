include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(cad_math_tests
        test_math.cpp
)

target_link_libraries(cad_math_tests
        PRIVATE
        cad_math
        Catch2::Catch2WithMain
)

target_compile_features(cad_math_tests PRIVATE cxx_std_23)

add_test(NAME cad_math_tests COMMAND cad_math_tests)