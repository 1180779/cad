
file(GLOB_RECURSE CAD_MATH_SOURCES
        include/cad_math/*.cpp
        include/cad_math/*.hpp
        include/cad_math/*.cxx
        include/cad_math/*.hxx
)

add_library(cad_math STATIC
        ${CAD_MATH_SOURCES}
)

target_include_directories(cad_math
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cad_math PUBLIC cxx_std_20)