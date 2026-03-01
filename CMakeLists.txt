cmake_minimum_required(VERSION 4.1)
project(cad)

set(CMAKE_CXX_STANDARD 20)
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

add_subdirectory(src/cad_math)
add_subdirectory(src/app)