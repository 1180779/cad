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

#include "gl.h"
#include "cad_math/helpers.h"

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

    const auto view = m_cameraStrategy->getView();
    const auto projection = m_cameraStrategy->getProjection();
    m_renderSystem.render(m_scene, view, projection);
}

void OpenGLWidget::resizeGL(const int width, const int height)
{
    QOpenGLWidget::resizeGL(width, height);
    m_cameraStrategy->syncAspectRatio(width, height);

    // qInfo() << "Resized to " << width << "x" << height;
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
    if (m_cameraStrategy->handleWheelEvent(event))
    {
        update();
    }
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_cameraStrategy->handleKeyPressEvent(event))
    {
        update();
        return;
    }

    switch (event->key())
    {
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
