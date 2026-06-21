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

add_subdirectory(src/cad_math)
add_subdirectory(src/common)
add_subdirectory(src/ellipse)
add_subdirectory(src/cad)

enable_testing()
add_subdirectory(tests)

# comment-style check, runnable on demand from CLion (green hammer) or the CLI.
# checks every tracked C/C++ source; the git pre-commit hook checks staged files only.
find_program(LUA_EXECUTABLE NAMES lua5.4 lua)
if (LUA_EXECUTABLE)
    add_custom_target(check_comments
            COMMAND ${CMAKE_COMMAND} -DLUA=${LUA_EXECUTABLE} -DROOT=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tools/lint/run_check_comments.cmake
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Checking comment style"
            VERBATIM)
endif ()