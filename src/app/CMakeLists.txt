add_executable(cad main.cpp
        OpenGLWidget.cpp
        OpenGLWidget.h
        ../cad_math/include/cad_math/math.h
        ../cad_math/src/math.cpp
)

target_link_libraries(cad
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGLWidgets
        cad_math
)
