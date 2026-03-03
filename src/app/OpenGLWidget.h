//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_RENDERINGWINDOW_H
#define CAD_RENDERINGWINDOW_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <memory>

#include "GL.h"
#include "ShaderProgram.h"
#include "quad.h"

class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit OpenGLWidget(QWidget* parent = nullptr);

    void paintGL() override;
    void resizeGL(int width, int height) override;
    void initializeGL() override;

private:
    void fillCpuBuffer();

    GLuint m_texture{};
    std::unique_ptr<ShaderProgram> m_shaderProgram;
    std::unique_ptr<quad> m_quad;
    std::vector<unsigned char> m_cpuBuffer{};
};


#endif //CAD_RENDERINGWINDOW_H