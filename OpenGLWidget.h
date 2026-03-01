//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_RENDERINGWINDOW_H
#define CAD_RENDERINGWINDOW_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QtOpenGL/QOpenGLShader>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
    public:
    explicit OpenGLWidget(QWidget* parent = nullptr);

    void paintGL() override;
    void resizeGL(int width, int height) override;
    void initializeGL() override;
};


#endif //CAD_RENDERINGWINDOW_H