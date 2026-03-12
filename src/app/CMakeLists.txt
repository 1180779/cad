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

add_custom_target(CopyShaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
        $<TARGET_FILE_DIR:cad>/shaders
        COMMENT "Copying shaders to output directory"
)

add_dependencies(cad CopyShaders)

if (WIN32)
    get_target_property(QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)

    add_custom_command(TARGET cad POST_BUILD
            COMMAND "${QT_BIN_DIR}/windeployqt.exe"
                    --no-translations
                    "$<TARGET_FILE:cad>"
            COMMENT "Running windeployqt"
    )
endif ()
