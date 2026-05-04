file(GLOB_RECURSE CAD_SOURCES
        src/*.cpp
        src/*.hpp
)

add_executable(cad ${CAD_SOURCES}
        src/Callbacks.hpp)

target_link_libraries(cad
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGLWidgets
        cad_math
        common
        TBB::tbb
)

target_include_directories(cad PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
