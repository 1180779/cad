FetchContent_MakeAvailable(Catch2)

add_executable(cad_math_tests
        Math.cpp
)

target_link_libraries(cad_math_tests
        PRIVATE
        cad_math
        Catch2::Catch2WithMain
)

target_compile_features(cad_math_tests PRIVATE cxx_std_23)

add_test(NAME cad_math_tests COMMAND cad_math_tests)

add_executable(cad_command_tests
        Commands.cpp
        ${CMAKE_SOURCE_DIR}/src/cad/src/commands/CommandStack.hpp
)

target_link_libraries(cad_command_tests
        PRIVATE
        cad_core
        Catch2::Catch2WithMain
)

target_compile_features(cad_command_tests PRIVATE cxx_std_23)

add_test(NAME cad_command_tests COMMAND cad_command_tests)

add_executable(cad_serialization_tests
        Serialization.cpp
)

target_link_libraries(cad_serialization_tests
        PRIVATE
        cad_core
        Catch2::Catch2WithMain
)

target_compile_definitions(cad_serialization_tests PRIVATE
        CAD_FORMAT_DIR="${CMAKE_SOURCE_DIR}/src/cad/format"
)

target_compile_features(cad_serialization_tests PRIVATE cxx_std_23)

add_test(NAME cad_serialization_tests COMMAND cad_serialization_tests)

add_executable(cad_scene_tests
        SceneLifecycle.cpp
)

target_link_libraries(cad_scene_tests
        PRIVATE
        cad_core
        Catch2::Catch2WithMain
)

target_compile_features(cad_scene_tests PRIVATE cxx_std_23)

add_test(NAME cad_scene_tests COMMAND cad_scene_tests)
