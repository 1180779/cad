FetchContent_MakeAvailable(valijson)

file(GLOB_RECURSE CAD_SOURCES
        src/*.cpp
        src/*.hpp
        src/*.cxx
        src/*.hxx
)
list(REMOVE_ITEM CAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

# AUTORCC compiles this; it must be listed as a target source. 
# Registers the vendored JetBrains icons + Mono font
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
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:cad>/format"
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

if (MINGW OR MSVC OR (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    get_target_property(QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
    if (MSVC)
        set(CAD_BUNDLE_PLATFORM windows-msvc)
        set(CAD_RUNTIME_NAME "MSVC runtime")
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
        include(InstallRequiredSystemLibraries)
        set(CAD_MSVC_RUNTIME_LIBRARIES "${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")
        get_filename_component(CAD_DUMPBIN_HINT "${CMAKE_LINKER}" DIRECTORY)
        find_program(CAD_DUMPBIN NAMES dumpbin HINTS "${CAD_DUMPBIN_HINT}" REQUIRED)
        set(CAD_DEPENDENCY_TOOL dumpbin)
        set(CAD_DEPENDENCY_COMMAND "${CAD_DUMPBIN}")
    elseif (MINGW)
        set(CAD_BUNDLE_PLATFORM windows-mingw)
        set(CAD_RUNTIME_NAME "MinGW runtime")
        set(CAD_DEPENDENCY_TOOL objdump)
        set(CAD_DEPENDENCY_COMMAND "${CMAKE_OBJDUMP}")
    else ()
        set(CAD_BUNDLE_PLATFORM linux-gcc)
    endif ()
    get_filename_component(CAD_COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_path(CAD_QT_LICENSE_DIR NAMES LICENSE
            HINTS "${QT_BIN_DIR}/../../../Licenses" "${QT_BIN_DIR}/../LICENSES"
            DOC "Directory of Qt license texts to include in the bundle" NO_DEFAULT_PATH)
    if (WIN32)
        set(CAD_BUNDLE_SCRIPT BundleWindows)
    else ()
        if (Qt6_VERSION VERSION_LESS 6.5)
            message(FATAL_ERROR "Linux bundles require Qt 6.5 or newer for runtime deployment.")
        endif ()
        set(CAD_BUNDLE_SCRIPT BundleLinux)
        include(GNUInstallDirs)
        set_target_properties(cad PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
        install(TARGETS cad RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT cad_bundle)
        install(TARGETS tbb
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT cad_bundle)
        install(DIRECTORY src/shaders format DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT cad_bundle)
        qt_generate_deploy_app_script(TARGET cad OUTPUT_SCRIPT CAD_DEPLOY_SCRIPT NO_TRANSLATIONS)
        install(SCRIPT ${CAD_DEPLOY_SCRIPT} COMPONENT cad_bundle)
    endif ()
    configure_file(${PROJECT_SOURCE_DIR}/cmake/${CAD_BUNDLE_SCRIPT}.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/${CAD_BUNDLE_SCRIPT}.cmake.in @ONLY)
    file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CAD_BUNDLE_SCRIPT}-$<CONFIG>.cmake
            INPUT ${CMAKE_CURRENT_BINARY_DIR}/${CAD_BUNDLE_SCRIPT}.cmake.in)
    add_custom_target(bundle
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/${CAD_BUNDLE_SCRIPT}-$<CONFIG>.cmake
            DEPENDS cad
            COMMENT "Creating ${CAD_BUNDLE_PLATFORM} bundle"
            VERBATIM)
endif ()
