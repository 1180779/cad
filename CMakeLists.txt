cmake_minimum_required(VERSION 4.1)
project(cad)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        OpenGLWidgets
        REQUIRED)

find_package(OpenGL REQUIRED)
find_package(TBB REQUIRED)

include(FetchContent)

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

add_subdirectory(src/cad_math)
add_subdirectory(src/common)
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