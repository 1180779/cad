//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.hpp"

#include <QMenu>

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

#include "GeometryFactory.hpp"
#include "GlCommon.hpp"
#include "PointRegistry.hpp"
#include "cad_math/helpers.hpp"
#include "components/CursorComponent.hpp"
#include "components/PointComponent.hpp"
#include "components/TransformComponent.hpp"
#include "cursor/GridPlanePlacementStrategy.hpp"

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_cursorPlacementStrategy(std::make_shared<GridPlanePlacementStrategy>(1 /*XY plane*/))
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
        const auto x0 = static_cast<cadm::cadf>(2.0 * static_cast<cadm::cadf>(rect.left()) / w - 1.0);
        const auto x1 = static_cast<cadm::cadf>(2.0 * static_cast<cadm::cadf>(rect.right()) / w - 1.0);
        const auto y0 = static_cast<cadm::cadf>(1.0 - 2.0 * static_cast<cadm::cadf>(rect.bottom()) / h);
        const auto y1 = static_cast<cadm::cadf>(1.0 - 2.0 * static_cast<cadm::cadf>(rect.top()) / h);
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
    gl->glEnable(GL_PROGRAM_POINT_SIZE);

    m_renderSystem.initialize();
}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePosition = event->pos();
    m_pressPosition = event->pos();

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

    if (event->button() == Qt::LeftButton &&
        (event->pos() - m_pressPosition).manhattanLength() <= s_clickRadiusPx)
    {
        const PointHandle hit = pickPoint(event->pos());

        if (hit == InvalidPointHandle &&
            event->modifiers() & Qt::ShiftModifier &&
            m_cursorPlacementStrategy)
        {
            if (Entity *cursor = m_scene.getActiveCursor())
            {
                auto *cam = m_cameraController.getActiveStrategy();
                const auto invView = cam->getView().inversedView();
                const auto invProj = cam->getInvProjection();
                if (const auto pos = m_cursorPlacementStrategy->resolve(event, width(), height(), invView, invProj); pos
                    .has_value())
                {
                    if (const auto transform = cursor->getComponent<TransformComponent>())
                        transform.value()->setTranslation(pos.value());
                    emit sceneChanged();
                    update();
                }
            }
            return;
        }

        if (hit == InvalidPointHandle && !(event->modifiers() & Qt::ShiftModifier)) // miss
        {
            for (auto &e : m_scene.getEntities())
                e->setSelected(false);
            m_scene.syncPointSelectionToRegistry();
            emit selectedEntityChanged(nullptr);
            emit viewportSelectionChanged();
            update();
        }
        if (hit != InvalidPointHandle) // hit
        {
            const bool additive = event->modifiers() & Qt::ShiftModifier;
            if (!additive)
            {
                for (auto &e : m_scene.getEntities())
                    e->setSelected(false);
                emit selectedEntityChanged(nullptr);
            }

            if (const auto entityOpt = m_scene.getEntityByPointHandle(hit))
            {
                Entity *entity = entityOpt.value();
                entity->setSelected(
                    additive
                        ? !entity->isSelected()
                        : true);
                m_scene.syncPointSelectionToRegistry();

                // only show properties when exactly one point entity is selected
                int selectedPointCount = 0;
                Entity *solePointEntity = nullptr;
                for (const auto &e : m_scene.getEntities())
                {
                    if (e->isSelected() && e->hasComponent<PointComponent>())
                    {
                        ++selectedPointCount;
                        solePointEntity = e.get();
                    }
                }
                emit selectedEntityChanged(
                    selectedPointCount == 1
                        ? solePointEntity
                        : nullptr);
            }
            emit viewportSelectionChanged();
            update();
            return;
        }
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

    case Qt::Key_C:
        {
            QMenu menu(this);
            menu.addAction("New Torus", [this] { emit createTorusRequested(); });
            menu.addAction("New Cursor", [this] { emit createCursorRequested(); });
            menu.addAction("New Point", [this] { emit createPointRequested(); });
            menu.exec(QCursor::pos());
        }
        break;

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

PointHandle OpenGLWidget::pickPoint(const QPoint screenPos) const
{
    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto projection = m_cameraController.getActiveStrategy()->getProjection();
    const auto &registry = m_scene.getPointRegistry();

    PointHandle best = InvalidPointHandle;
    int bestDist = s_clickRadiusPx * s_clickRadiusPx + 1;

    for (const PointHandle h : registry.aliveHandles())
    {
        const auto screen = cadm::projectToScreenGL(
            registry.getPosition(h),
            view,
            projection,
            width(),
            height());
        if (!screen.has_value())
            continue;

        const int dx = screen->x - screenPos.x();
        const int dy = screen->y - screenPos.y();
        if (const int dist = dx * dx + dy * dy; dist < bestDist)
        {
            bestDist = dist;
            best = h;
        }
    }

    return best;
}

void OpenGLWidget::performBoxSelect()
{
    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto projection = m_cameraController.getActiveStrategy()->getProjection();
    const QRect rect = QRect(m_boxSelectStart, m_boxSelectCurrent).normalized();

    const auto &registry = m_scene.getPointRegistry();
    for (const auto &e : m_scene.getEntities())
    {
        cadm::vec3 worldPos;
        if (const auto pc = e->getComponent<PointComponent>())
            worldPos = registry.getPosition(pc.value()->m_handle);
        else if (const auto transform = e->getComponent<TransformComponent>())
            worldPos = transform.value()->getTranslation();
        else
            continue;

        const auto screen = cadm::projectToScreenGL(worldPos, view, projection, width(), height());
        e->setSelected(screen.has_value() && rect.contains(screen->x, screen->y));
    }

    m_scene.syncPointSelectionToRegistry();
    emit selectedEntityChanged(nullptr);
    emit viewportSelectionChanged();
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
