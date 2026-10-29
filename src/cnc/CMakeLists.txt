file(GLOB_RECURSE CNC_SOURCES
        src/*.cxx
        include/*.hxx
)

add_library(cnc_core STATIC
        ${CNC_SOURCES}
        include/cnc/Snapshot.hxx
)

target_include_directories(cnc_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cnc_core
        PUBLIC
        cad_math::cad_math
        cad_graphics::cad_graphics
)

target_compile_features(cnc_core PUBLIC cxx_std_20)
