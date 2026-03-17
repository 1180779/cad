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

#include "cameraFactory.hpp"
#include "components/camera.hpp"
#include "components/transform.h"

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

    const auto camera = m_mainCamera->getComponent<CameraComponent>();
    if (!camera)
    {
        qWarning() << "No camera component found for main camera entity. Skipping rendering" << __FILE__ <<
            ", " << __LINE__;
        return;
    }
    const auto pCamera = camera.value();

    const auto view = cadm::mat4::lookAtRH(pCamera->m_position, pCamera->m_target, pCamera->up());
    const double height = pCamera->m_orthoHeight;
    const double width = height * pCamera->m_aspectRatio;

    const auto projection = cadm::mat4::ortho(
        -width / 2.0,
        width / 2.0,
        -height / 2.0,
        height / 2.0,
        pCamera->m_nearPlane,
        pCamera->m_farPlane);
    m_renderSystem.render(m_scene, view, projection);
}

void OpenGLWidget::resizeGL(const int width, const int height)
{
    QOpenGLWidget::resizeGL(width, height);
    const auto cameraComp = m_mainCamera->getComponent<CameraComponent>();
    if (!cameraComp)
        return;
    cameraComp.value()->m_aspectRatio = static_cast<cadm::cadf>(width) / static_cast<cadm::cadf>(height);

    // qInfo() << "Resized to " << width << "x" << height;
}

void OpenGLWidget::initializeGL()
{
    const auto gl = GL();
    gl->glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const CameraFactory cameraFactory(m_scene);
    m_mainCamera = cameraFactory.createArcBallCamera({0, 0, 10}, {}, cadm::vec3::unitY());
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
    const int delta = event->angleDelta().y();
    if (delta == 0)
        return;

    const auto camera = m_mainCamera->getComponent<CameraComponent>();
    if (!camera)
        return;
    const auto pCamera = camera.value();

    const auto pos = event->position();
    const auto nx = pos.x() / width() * 2.0 - 1.0;
    const auto ny = 1.0 - pos.y() / height() * 2.0;

    const auto oldHeight = pCamera->m_orthoHeight;
    const auto oldWidth = oldHeight * pCamera->m_aspectRatio;

    if (delta > 0)
    {
        pCamera->m_orthoHeight /= m_zoomFactor;
    }
    else
    {
        pCamera->m_orthoHeight *= m_zoomFactor;
    }

    const auto newHeight = pCamera->m_orthoHeight;
    const auto newWidth = newHeight * pCamera->m_aspectRatio;

    const auto deltaX = (oldWidth - newWidth) / 2.0 * nx;
    const auto deltaY = (oldHeight - newHeight) / 2.0 * ny;

    const auto right = pCamera->right();
    const auto up = pCamera->up();

    const auto translation = right * deltaX + up * deltaY;
    pCamera->m_position += translation;
    pCamera->m_target += translation;

    if (const auto transform = m_mainCamera->getComponent<TransformComponent>())
    {
        transform.value()->setTranslation(pCamera->m_position);
    }

    update();
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_W:
    case Qt::UpArrow:
        {
            const auto camera = m_mainCamera->getComponent<CameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->m_orthoHeight;
            const auto up = pCamera->up();
            pCamera->m_position += up * step;
            pCamera->m_target += up * step;
            if (auto transform = m_mainCamera->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->m_position);
            }
            update();
            break;
        }
    case Qt::Key_S:
    case Qt::DownArrow:
        {
            const auto camera = m_mainCamera->getComponent<CameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->m_orthoHeight;
            const auto up = pCamera->up();
            pCamera->m_position -= up * step;
            pCamera->m_target -= up * step;
            if (auto transform = m_mainCamera->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->m_position);
            }
            update();
            break;
        }
    case Qt::Key_A:
    case Qt::LeftArrow:
        {
            const auto camera = m_mainCamera->getComponent<CameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->m_orthoHeight;
            const auto right = pCamera->right();
            pCamera->m_position -= right * step;
            pCamera->m_target -= right * step;
            if (auto transform = m_mainCamera->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->m_position);
            }
            update();
            break;
        }
    case Qt::Key_D:
    case Qt::RightArrow:
        {
            const auto camera = m_mainCamera->getComponent<CameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->m_orthoHeight;
            const auto right = pCamera->right();
            pCamera->m_position += right * step;
            pCamera->m_target += right * step;
            if (auto transform = m_mainCamera->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->m_position);
            }
            update();
            break;
        }
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
