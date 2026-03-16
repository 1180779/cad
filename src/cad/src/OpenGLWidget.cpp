//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.h"

#include <QWheelEvent>

// undefine Qt's emit macro to avoid conflicts with TBB
#ifdef emit
#define QT_EMIT_DEFINED
#undef emit
#endif

#include <execution>

// restore Qt's emit macro
#ifdef QT_EMIT_DEFINED
#define emit
#undef QT_EMIT_DEFINED
#endif

#include "geometryFactory.h"
#include "gl.h"
#include "cad_math/helpers.h"
#include "OpenGLWidget.h"

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

OpenGLWidget::~OpenGLWidget() = default;

void OpenGLWidget::paintGL()
{
    const auto gl = GL();
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_renderSystem.render(m_scene, m_camera);
}

void OpenGLWidget::resizeGL(const int width, const int height)
{
    QOpenGLWidget::resizeGL(width, height);
    m_camera.setAspectRatio(static_cast<cadm::cadf>(width) / static_cast<cadm::cadf>(height));

    // qInfo() << "Resized to " << width << "x" << intheight;
}

void OpenGLWidget::initializeGL()
{
    const auto gl = GL();
    gl->glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_renderSystem.initialize();
}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePosition = event->pos();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    const auto currentPos = event->pos();
    const auto delta = currentPos - m_lastMousePosition;
    m_lastMousePosition = currentPos;
}

void OpenGLWidget::wheelEvent(QWheelEvent *event)
{
    // const int delta = event->angleDelta().y();
    // if (delta == 0)
    //     return;
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_W:
    case Qt::UpArrow:
        break;
    case Qt::Key_S:
    case Qt::DownArrow:
        break;
    case Qt::Key_A:
    case Qt::LeftArrow:
        break;
    case Qt::Key_D:
    case Qt::RightArrow:
        break;
    case Qt::Key_Q:
        break;
    case Qt::Key_E:
        break;
    case Qt::Key_X:
        if (event->isAutoRepeat())
            return;
        m_xPressed = true;
        return;
    case Qt::Key_Y:
        if (event->isAutoRepeat())
            return;
        m_yPressed = true;
        return;
    case Qt::Key_Z:
        if (event->isAutoRepeat())
            return;
        m_zPressed = true;
        return;
    default:
        QOpenGLWidget::keyPressEvent(event);
        return;
    }
}

void OpenGLWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
    {
        switch (event->key())
        {
        case Qt::Key_X:
            m_xPressed = false;
            break;
        case Qt::Key_Y:
            m_yPressed = false;
            break;
        case Qt::Key_Z:
            m_zPressed = false;
            break;
        default:
            QOpenGLWidget::keyReleaseEvent(event);
        }
    }
}

bool OpenGLWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        const auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        keyPressEvent(keyEvent);
        return true;
    }
    if (event->type() == QEvent::KeyRelease)
    {
        const auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        keyReleaseEvent(keyEvent);
        return true;
    }
    return QObject::eventFilter(obj, event);
}