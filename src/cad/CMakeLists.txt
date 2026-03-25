add_executable(cad src/main.cpp
        src/OpenGLWidget.cpp
        src/OpenGLWidget.hpp
        src/quad.cpp
        src/quad.hpp
        src/gl.cpp
        src/gl.hpp
        src/shaderProgram.cpp
        src/shaderProgram.hpp
        src/entities/entity.hpp
        src/components/geometry.hpp
        src/components/transform.hpp
        src/renderSystem.hpp
        src/scene.hpp
        src/geometryFactory.hpp
        src/scene.cpp
        src/scene.cpp
        src/renderSystem.cpp
        src/checkMacros.hpp
        src/geometryFactory.cpp
        src/components/geometry.cpp
        src/components/transform.cpp
        src/gui/ComponentWidget.hpp
        src/gui/ComponentWidget.cpp
        src/gui/TransformWidget.hpp
        src/gui/TransformWidget.cpp
        src/gui/TorusWidget.hpp
        src/gui/TorusWidget.cpp
        src/gui/EntityPropertiesWidget.hpp
        src/gui/EntityPropertiesWidget.cpp
        src/gui/SceneHierarchyWidget.hpp
        src/gui/SceneHierarchyWidget.cpp
        src/components/camera.hpp
        src/components/camera.cpp
        src/cameraFactory.cpp
        src/cameraFactory.hpp
        src/camera/ICameraStrategy.hpp
        src/camera/projectionCameraStrategy.cpp
        src/camera/projectionCameraStrategy.hpp
        src/gui/projectionCameraWidget.cpp
        src/gui/projectionCameraWidget.hpp
        src/camera/cadCameraStrategy.cpp
        src/camera/cadCameraStrategy.hpp
        src/components/ICamera.hpp
        src/components/cadCameraCompoonent.hpp
        src/components/cadCameraComponent.cpp
        src/gui/cadCameraWidget.cpp
        src/gui/cadCameraWidget.hpp
        src/gui/GridSettingsWidget.hpp
        src/gui/GridSettingsWidget.cpp
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
