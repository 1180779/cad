cmake_minimum_required(VERSION 4.1)
project(cad)

# Distributable archives need Qt's runtime deployment API (Qt >= 6.5 on Linux).
# Bundling is opt-in so development builds work on older Qt versions; pass
# -DCAD_ENABLE_BUNDLE=ON to build the release archives.
option(CAD_ENABLE_BUNDLE "Build the distributable bundle target" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        OpenGL
        OpenGLWidgets
        REQUIRED)

find_package(OpenGL REQUIRED)

include(FetchContent)

set(TBB_TEST OFF CACHE BOOL "Don't build TBB tests" FORCE)
set(TBB_STRICT OFF CACHE BOOL "Don't treat TBB compiler warnings as errors" FORCE)
set(TBB_EXAMPLES OFF CACHE BOOL "Don't build TBB examples" FORCE)

FetchContent_Declare(
        oneTBB
        GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
        GIT_TAG v2021.13.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(oneTBB)

FetchContent_Declare(
        valijson
        GIT_REPOSITORY https://github.com/tristanpenman/valijson.git
        GIT_TAG v1.0.6
        GIT_SHALLOW TRUE
        GIT_SUBMODULES ""
)
set(valijson_BUILD_TESTS OFF CACHE BOOL "Don't build valijson tests" FORCE)
set(valijson_BUILD_EXAMPLES OFF CACHE BOOL "Don't build valijson examples" FORCE)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4
)

FetchContent_Declare(
        cad_math
        GIT_REPOSITORY https://github.com/1180779/cad_math.git
        GIT_TAG v1.0.1
)
FetchContent_MakeAvailable(cad_math)

FetchContent_Declare(
        cad_graphics
        GIT_REPOSITORY https://github.com/1180779/cad_graphics.git
        GIT_TAG v1.1.0
)
FetchContent_MakeAvailable(cad_graphics)

FetchContent_Declare(
        cad_common
        GIT_REPOSITORY https://github.com/1180779/cad_common.git
        GIT_TAG v1.1.1
)
FetchContent_MakeAvailable(cad_common)

add_subdirectory(src/cnc)
add_subdirectory(src/cnc_gpu)
add_subdirectory(src/cnc_ui)
add_subdirectory(src/ellipse)
add_subdirectory(src/cad)

enable_testing()
add_subdirectory(tests)

find_program(LUA_EXECUTABLE NAMES lua5.4 lua)
if (LUA_EXECUTABLE)
    add_custom_target(check_comments
            COMMAND ${CMAKE_COMMAND} -DLUA=${LUA_EXECUTABLE} -DROOT=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tools/lint/run_check_comments.cmake
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Checking comment style"
            VERBATIM)
endif ()
