add_library(cnc_ui STATIC
        src/CncRenderSystem.cxx
)

target_include_directories(cnc_ui
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cnc_ui
        PUBLIC
        cnc_core
        cnc_gpu
        Qt6::Gui
        Qt6::OpenGL
        OpenGL::GL
)

target_compile_features(cnc_ui PUBLIC cxx_std_23)
