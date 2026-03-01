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
}

void OpenGLWidget::resizeGL(int width, int height)
{
    QOpenGLWidget::resizeGL(width, height);
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
