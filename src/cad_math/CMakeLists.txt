add_library(cad_math STATIC
        include/cad_math/vec_base.hpp
        include/cad_math/vec3.hpp
        include/cad_math/vec4.hpp
        include/cad_math/mat_base.hpp
        include/cad_math/mat4.hpp
        include/cad_math/common.hpp
        include/cad_math/vec3i.hpp
        include/cad_math/mat3.hpp
        include/cad_math/vec2.hpp
        include/cad_math/helpers.hpp
        include/cad_math/vec2i.hpp
        include/cad_math/ray4.hpp
)

target_include_directories(cad_math
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cad_math PUBLIC cxx_std_20)