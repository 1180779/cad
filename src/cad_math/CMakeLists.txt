add_library(cad_math STATIC
        src/math.cpp
        include/cad_math/math.h
)

target_include_directories(cad_math
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cad_math PUBLIC cxx_std_20)