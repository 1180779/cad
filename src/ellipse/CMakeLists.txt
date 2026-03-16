add_executable(ellipse src/main.cpp
        src/OpenGLWidget.cpp
        src/OpenGLWidget.h
        src/quad.cpp
        src/quad.h
        src/GL.cpp
        src/GL.h
        src/shaderProgram.cpp
        src/shaderProgram.h
        src/camera.h
        src/camera.cpp
)

target_link_libraries(ellipse
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGLWidgets
        cad_math
        common
        TBB::tbb
)

add_custom_target(EllipseCopyShaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
        $<TARGET_FILE_DIR:ellipse>/shaders
        COMMENT "Copying shaders to output directory"
)

add_dependencies(ellipse EllipseCopyShaders)

if (WIN32)
    get_target_property(QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)

    add_custom_command(TARGET ellipse POST_BUILD
            COMMAND "${QT_BIN_DIR}/windeployqt.exe"
            --no-translations
            "$<TARGET_FILE:ellipse>"
            COMMENT "Running windeployqt"
    )
endif ()
