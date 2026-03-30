//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.hpp"

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

#include "gl.hpp"
#include "cad_math/helpers.hpp"
#include "components/transform.hpp"

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

    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto projection = m_cameraController.getActiveStrategy()->getProjection();
    m_renderSystem.render(m_scene, view, projection);

    if (m_boxSelecting)
    {
        const QRect rect = QRect(m_boxSelectStart, m_boxSelectCurrent).normalized();
        const auto w = static_cast<cadm::cadf>(width());
        const auto h = static_cast<cadm::cadf>(height());
        const auto x0 = 2.0 * static_cast<cadm::cadf>(rect.left()) / w - 1.0;
        const auto x1 = 2.0 * static_cast<cadm::cadf>(rect.right()) / w - 1.0;
        const auto y0 = 1.0 - 2.0 * static_cast<cadm::cadf>(rect.bottom()) / h;
        const auto y1 = 1.0 - 2.0 * static_cast<cadm::cadf>(rect.top()) / h;
        m_renderSystem.renderSelectionRect(x0, y0, x1, y1);
    }
}

void OpenGLWidget::resizeGL(const int width, const int height)
{
    QOpenGLWidget::resizeGL(width, height);
    m_cameraController.getActiveStrategy()->syncAspectRatio(width, height);
}

void OpenGLWidget::initializeGL()
{
    const auto gl = GL();
    gl->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_renderSystem.initialize();
}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePosition = event->pos();

    if (m_boxSelectMode && event->button() == m_boxSelectMouseButton)
    {
        m_boxSelecting = true;
        m_boxSelectStart = event->pos();
        m_boxSelectCurrent = event->pos();
        return;
    }

    if (m_cameraController.getActiveStrategy()->handleMousePressEvent(event))
    {
        update();
    }
    m_lastMousePosition = event->pos();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    const auto currentPos = event->pos();
    const auto delta = currentPos - m_lastMousePosition;
    m_lastMousePosition = currentPos;

    if (m_boxSelecting)
    {
        m_boxSelectCurrent = currentPos;
        update();
        return;
    }

    if (m_cameraController.getActiveStrategy()->handleMouseMoveEvent(event, delta))
    {
        update();
    }
}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_boxSelecting && event->button() == m_boxSelectMouseButton)
    {
        m_boxSelecting = false;
        performBoxSelect();
        update();
        return;
    }

    if (m_cameraController.getActiveStrategy()->handleMouseReleaseEvent(event))
    {
        update();
    }
}

void OpenGLWidget::wheelEvent(QWheelEvent *event)
{
    if (m_cameraController.getActiveStrategy()->handleWheelEvent(event))
    {
        update();
    }
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_cameraController.getActiveStrategy()->handleKeyPressEvent(event))
    {
        update();
        return;
    }

    switch (event->key())
    {
    case Qt::Key_N:
        m_cameraController.switchToNext(width(), height());
        update();
        return;
    case Qt::Key_Delete:
        deleteSelectedEntities();
        return;
    case Qt::Key_B:
        if (event->isAutoRepeat()) return;
        m_boxSelectMode = true;
        return;
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
        case Qt::Key_B:
            if (!m_boxSelecting)
                m_boxSelectMode = false;
            break;
        default:
            QOpenGLWidget::keyReleaseEvent(event);
        }
    }
}

void OpenGLWidget::deleteSelectedEntities()
{
    std::vector<EntityID> toDelete;
    for (const auto &e : m_scene.getEntities())
        if (e->isSelected())
            toDelete.push_back(e->getId());

    for (const EntityID id : toDelete)
        m_scene.removeEntity(id);

    if (!toDelete.empty())
    {
        emit sceneChanged();
        update();
    }
}

void OpenGLWidget::performBoxSelect()
{
    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto projection = m_cameraController.getActiveStrategy()->getProjection();
    const QRect rect = QRect(m_boxSelectStart, m_boxSelectCurrent).normalized();

    for (const auto &e : m_scene.getEntities())
    {
        const auto transform = e->getComponent<TransformComponent>();
        if (!transform) continue;
        const auto screen = cadm::projectToScreenGL(
            transform.value()->getTranslation(),
            view,
            projection,
            width(),
            height());
        e->setSelected(screen.has_value() && rect.contains(screen->x, screen->y));
    }
    emit selectedEntityChanged(nullptr);
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