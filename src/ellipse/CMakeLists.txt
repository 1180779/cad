add_executable(ellipse src/main.cpp
        src/OpenGlWidget.cpp
        src/OpenGlWidget.hpp
        src/Quad.cpp
        src/Quad.hpp
        src/Gl.cpp
        src/Gl.hpp
        src/ShaderProgram.cpp
        src/ShaderProgram.hpp
        src/Camera.hpp
        src/Camera.cpp
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

target_include_directories(ellipse PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
