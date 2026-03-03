//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.h"

OpenGLWidget::OpenGLWidget(QWidget* parent) : QOpenGLWidget(parent)
{
}

void OpenGLWidget::paintGL()
{
    const auto gl = GL();
    fillCpuBuffer();
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width(), height(), GL_RGB, GL_UNSIGNED_BYTE, m_cpuBuffer.data());

    gl->glClear(GL_COLOR_BUFFER_BIT);
    m_shaderProgram->bind();
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    m_shaderProgram->setUniformValue("screenTexture", 0);
    m_quad->draw();
    m_shaderProgram->release();
}

void OpenGLWidget::resizeGL(int width, int height)
{
    QOpenGLWidget::resizeGL(width, height);
    const size_t bufferSize = width * height * 3;
    m_cpuBuffer.resize(bufferSize);

    const auto gl = GL();
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    qInfo() << "Resized to " << width << "x" << height;
}

void OpenGLWidget::initializeGL()
{
    const auto gl = GL();
    gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // for rgb the cpu buffer is not aligned, which leads to artifacts
    gl->glClearColor(0.0f, 1.0f, 1.0f, 1.0f);

    m_quad = std::make_unique<quad>();

    m_shaderProgram = std::make_unique<ShaderProgram>();
    m_shaderProgram->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/vertexShader.vert");
    m_shaderProgram->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag");
    m_shaderProgram->compile();

    // create texture
    gl->glGenTextures(1, &m_texture);
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width(), height(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void OpenGLWidget::fillCpuBuffer()
{
    const int w = width();
    const int h = height();
    if (m_cpuBuffer.size() != w * h * 3)
    {
        qWarning() << "The cpu buffer size is not correct!";
        return;
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int r = (x * x + y * y) % 256;
            const int g = (x * 3 + y * 7) % 256;
            const int b = (x ^ y) & 255;

            const int i = (y * w + x) * 3;
            m_cpuBuffer[i + 0] = static_cast<unsigned char>(r);
            m_cpuBuffer[i + 1] = static_cast<unsigned char>(g);
            m_cpuBuffer[i + 2] = static_cast<unsigned char>(b);
        }
    }
}
