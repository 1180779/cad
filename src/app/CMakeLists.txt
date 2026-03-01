add_executable(cad main.cpp
        OpenGLWidget.cpp
        OpenGLWidget.h
        ../cad_math/include/cad_math/vec_base.h
        ../../tests/test_math.cpp
)

target_link_libraries(cad
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGLWidgets
        cad_math
)
