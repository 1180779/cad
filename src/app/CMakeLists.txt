add_executable(cad main.cpp
        OpenGLWidget.cpp
        OpenGLWidget.h
        quad.cpp
        quad.h
        GL.cpp
        GL.h
        ShaderProgram.cpp
        ShaderProgram.h
)

target_link_libraries(cad
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGLWidgets
        cad_math
)

# Create a custom target to copy shaders.
# This runs every build to ensure shaders are up-to-date even if C++ code hasn't changed.
add_custom_target(CopyShaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:cad>/shaders
        COMMENT "Copying shaders to output directory"
)

# Make the executable depend on the copying step
add_dependencies(cad CopyShaders)
