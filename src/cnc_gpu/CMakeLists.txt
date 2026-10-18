add_library(cnc_gpu STATIC
        src/GpuHeightMap.cxx
)

target_include_directories(cnc_gpu
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cnc_gpu
        PUBLIC
        cnc_core
        Qt6::Gui
        Qt6::OpenGL
        OpenGL::GL
)

target_compile_features(cnc_gpu PUBLIC cxx_std_20)
