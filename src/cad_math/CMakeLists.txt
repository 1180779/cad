add_library(cad_math STATIC
        include/cad_math/vec_base.h
        include/cad_math/vec3.h
        include/cad_math/vec4.h
        include/cad_math/mat_base.h
        include/cad_math/mat4.h
        include/cad_math/common.h
        include/cad_math/vec3i.h
        include/cad_math/mat3.h
        include/cad_math/vec2.h
        include/cad_math/helpers.h
        include/cad_math/vec2i.h
        include/cad_math/ray4.h
)

target_include_directories(cad_math
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cad_math PUBLIC cxx_std_20)