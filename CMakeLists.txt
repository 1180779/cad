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