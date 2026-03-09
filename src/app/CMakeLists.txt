add_executable(cad src/main.cpp
        src/OpenGLWidget.cpp
        src/OpenGLWidget.h
        src/quad.cpp
        src/quad.h
        src/GL.cpp
        src/GL.h
        src/ShaderProgram.cpp
        src/ShaderProgram.h
        src/Camera.h
        src/Camera.cpp
)

target_link_libraries(cad
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGLWidgets
        cad_math
        common
        TBB::tbb
)

# Create a custom target to copy shaders.
# This runs every build to ensure shaders are up-to-date even if C++ code hasn't changed.
add_custom_target(CopyShaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
        $<TARGET_FILE_DIR:cad>/shaders
        COMMENT "Copying shaders to output directory"
)

# Make the executable depend on the copying step
add_dependencies(cad CopyShaders)
