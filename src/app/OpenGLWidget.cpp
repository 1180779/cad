//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.h"

OpenGLWidget::OpenGLWidget(QWidget* parent) : QOpenGLWidget(parent)
{

}

void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    m_shaderProgram->bind();
    m_quad->draw();
    m_shaderProgram->release();
}

void OpenGLWidget::resizeGL(int width, int height)
{
    QOpenGLWidget::resizeGL(width, height);
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);

    m_quad = std::make_unique<quad>();

    m_shaderProgram = std::make_unique<ShaderProgram>();
    m_shaderProgram->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/vertexShader.vert");
    m_shaderProgram->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag");
    m_shaderProgram->compile();
}
