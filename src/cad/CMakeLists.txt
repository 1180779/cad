add_executable(cad src/main.cpp
        src/OpenGLWidget.cpp
        src/OpenGLWidget.h
        src/quad.cpp
        src/quad.h
        src/gl.cpp
        src/gl.h
        src/shaderProgram.cpp
        src/shaderProgram.h
        src/entities/entity.h
        src/components/geometry.h
        src/components/transform.h
        src/components/material.h
        src/renderSystem.h
        src/scene.h
        src/geometryFactory.h
        src/scene.cpp
        src/scene.cpp
        src/renderSystem.cpp
        src/checkMacros.hpp
        src/geometryFactory.cpp
        src/components/geometry.cpp
        src/components/transform.cpp
        src/gui/ComponentWidget.h
        src/gui/ComponentWidget.cpp
        src/gui/TransformWidget.h
        src/gui/TransformWidget.cpp
        src/gui/TorusWidget.h
        src/gui/TorusWidget.cpp
        src/gui/EntityPropertiesWidget.h
        src/gui/EntityPropertiesWidget.cpp
        src/gui/SceneHierarchyWidget.h
        src/gui/SceneHierarchyWidget.cpp
        src/components/camera.hpp
        src/cameraFactory.cpp
        src/cameraFactory.hpp
        src/camera/ICameraStrategy.hpp
        src/camera/cadCameraStrategy.cpp
        src/camera/cadCameraStrategy.hpp
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

add_custom_target(CadCopyShaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
        $<TARGET_FILE_DIR:cad>/shaders
        COMMENT "Copying shaders to output directory"
)

add_dependencies(cad CadCopyShaders)

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
