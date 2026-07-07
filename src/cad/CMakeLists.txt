FetchContent_MakeAvailable(valijson)

file(GLOB_RECURSE CAD_SOURCES
        src/*.cpp
        src/*.hpp
        src/*.cxx
        src/*.hxx
)
list(REMOVE_ITEM CAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

# AUTORCC compiles this; it must be listed as a target source (the GLOB above
# only picks up .cpp/.hpp). Registers the vendored JetBrains icons + Mono font.
set(CAD_RESOURCES resources/resources.qrc)

add_library(cad_core ${CAD_SOURCES} ${CAD_RESOURCES})

target_link_libraries(cad_core PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGLWidgets
        cad_math
        common
        TBB::tbb
        ValiJSON::valijson
)

target_include_directories(cad_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(cad src/main.cpp)

target_link_libraries(cad PRIVATE cad_core)

add_custom_target(CadCopyShaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
        $<TARGET_FILE_DIR:cad>/shaders
        COMMENT "Copying shaders to output directory"
)

add_custom_target(CadCopyFormat
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/format/schema.json
        $<TARGET_FILE_DIR:cad>/format/schema.json
        COMMENT "Copying scene JSON schema to output directory"
)

add_dependencies(cad CadCopyShaders CadCopyFormat)

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
