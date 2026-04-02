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
    : QOpenGLWidget(parent), m_cursorPlacementStrategy(std::make_unique<GridPlanePlacementStrategy>(1 /*XY plane*/))
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
    const auto invVP = view.inversedView() * m_cameraController.getActiveStrategy()->getInvProjection();
    m_renderSystem.render(m_scene, view, projection, invVP);

    if (const auto pivot = computePivot())
        m_renderSystem.renderPivotMarker(pivot.value(), view, projection);

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
    if (event->button() == Qt::LeftButton)
        m_leftMouseDown = true;
    else if (event->button() == Qt::RightButton)
        m_rightMouseDown = true;

    if (m_transformMode != TransformMode::None)
        return;

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

    if (m_transformMode != TransformMode::None)
    {
        applyTransform(currentPos);
        update();
        return;
    }

    if (m_boxSelecting)
    {
        m_boxSelectCurrent = currentPos;
        update();
        return;
    }

    if (m_leftMouseDown &&
        event->modifiers() & Qt::ShiftModifier &&
        m_cursorPlacementStrategy)
    {
        if (Entity *cursor = m_scene.getActiveCursor())
        {
            auto *cam = m_cameraController.getActiveStrategy();
            const auto invView = cam->getView().inversedView();
            const auto invProj = cam->getInvProjection();
            if (const auto pos = m_cursorPlacementStrategy->resolve(event, width(), height(), invView, invProj);
                pos.has_value())
            {
                if (const auto transform = cursor->getComponent<TransformComponent>())
                    transform.value()->setTranslation(pos.value());
                emit sceneChanged();
                update();
            }
        }
        return;
    }

    if (m_cameraController.getActiveStrategy()->handleMouseMoveEvent(event, delta))
    {
        update();
    }
}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_leftMouseDown = false;
    else if (event->button() == Qt::RightButton)
        m_rightMouseDown = false;

    if (m_transformMode != TransformMode::None && event->button() == Qt::LeftButton)
    {
        confirmTransform();
        update();
        return;
    }

    if (m_boxSelecting && event->button() == m_boxSelectMouseButton)
    {
        m_boxSelecting = false;
        performBoxSelect();
        update();
        return;
    }

    const auto diff = event->pos() - m_pressPosition;
    if (const auto dist2 = diff.x() * diff.x() + diff.y() * diff.y();
        event->button() == Qt::LeftButton &&
        dist2 <= s_clickRadiusPx * s_clickRadiusPx)
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

            if (const auto entity = m_scene.getEntityByPointHandle(hit))
            {
                Entity *pEntity = entity.value();
                pEntity->setSelected(
                    additive
                        ? !pEntity->isSelected()
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
    case Qt::Key_Escape:
        if (m_transformMode != TransformMode::None)
        {
            cancelTransform();
            update();
        }
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_transformMode != TransformMode::None)
        {
            confirmTransform();
            update();
        }
        return;
    case Qt::Key_G:
        if (event->isAutoRepeat()) return;
        if (m_transformMode == TransformMode::Translate)
            cancelTransform();
        else
        {
            if (m_transformMode != TransformMode::None)
                confirmTransform();
            beginTransform(TransformMode::Translate);
        }
        update();
        return;
    case Qt::Key_R:
        if (event->isAutoRepeat()) return;
        if (m_transformMode == TransformMode::Rotate)
            cancelTransform();
        else
        {
            if (m_transformMode != TransformMode::None)
                confirmTransform();
            beginTransform(TransformMode::Rotate);
        }
        update();
        return;
    case Qt::Key_S:
        if (event->isAutoRepeat()) return;
        if (m_transformMode == TransformMode::Scale)
            cancelTransform();
        else
        {
            if (m_transformMode != TransformMode::None)
                confirmTransform();
            beginTransform(TransformMode::Scale);
        }
        update();
        return;
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
        if (m_transformMode == TransformMode::Rotate)
            emit transformModeChanged(m_transformMode, "X");
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
        if (m_transformMode == TransformMode::Rotate)
            emit transformModeChanged(m_transformMode, "Y");
        return;
    case Qt::Key_Z:
        if (event->isAutoRepeat())
            return;
        m_zPressed = true;
        if (m_transformMode == TransformMode::Rotate)
            emit transformModeChanged(m_transformMode, "Z");
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
            if (m_transformMode == TransformMode::Rotate)
                emit transformModeChanged(
                m_transformMode,
                m_yPressed
                    ? "Y"
                    : m_zPressed
                    ? "Z"
                    : "");
            break;
        case Qt::Key_Y:
            m_yPressed = false;
            if (m_transformMode == TransformMode::Rotate)
                emit transformModeChanged(
                m_transformMode,
                m_xPressed
                    ? "X"
                    : m_zPressed
                    ? "Z"
                    : "");
            break;
        case Qt::Key_Z:
            m_zPressed = false;
            if (m_transformMode == TransformMode::Rotate)
                emit transformModeChanged(
                m_transformMode,
                m_xPressed
                    ? "X"
                    : m_yPressed
                    ? "Y"
                    : "");
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
    const Entity *activeCursor = m_scene.getActiveCursor();
    std::vector<EntityID> toDelete;
    for (const auto &e : m_scene.getEntities())
    {
        if (!e->isSelected())
            continue;
        if (m_cameraController.isManagedCamera(e->getId()))
            continue;
        if (e.get() == activeCursor)
            continue;
        toDelete.push_back(e->getId());
    }

    for (const EntityID id : toDelete)
        removeEntity(id);

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

std::optional<cadm::vec3> OpenGLWidget::computePivot() const
{
    if (m_pivotMode == PivotMode::ActiveCursor)
    {
        if (Entity *cursor = m_scene.getActiveCursor())
            if (const auto tc = cursor->getComponent<TransformComponent>())
                return tc.value()->getTranslation();
        return std::nullopt;
    }

    cadm::vec3 sum{};
    int count = 0;
    const auto &registry = m_scene.getPointRegistry();
    for (const auto &e : m_scene.getEntities())
    {
        if (!e->isSelected()) continue;
        if (const auto pc = e->getComponent<PointComponent>())
            sum = sum + registry.getPosition(pc.value()->m_handle);
        else if (const auto tc = e->getComponent<TransformComponent>())
            sum = sum + tc.value()->getTranslation();
        else
            continue;
        ++count;
    }
    if (count <= 1) return std::nullopt;
    return sum * (static_cast<cadm::cadf>(1.0) / static_cast<cadm::cadf>(count));
}

void OpenGLWidget::beginTransform(const TransformMode mode)
{
    const auto pivot = computePivot();
    if (!pivot.has_value())
        return;

    m_transformPivot = pivot.value();
    m_transformStartMousePos = mapFromGlobal(QCursor::pos());
    m_transformSnapshots.clear();

    const auto &registry = m_scene.getPointRegistry();
    for (const auto &e : m_scene.getEntities())
    {
        if (!e->isSelected()) continue;

        EntitySnapshot snap;
        if (!snap.fillFromEntity(registry, e.get()))
            continue;

        m_transformSnapshots.push_back(snap);
    }

    if (m_transformSnapshots.empty())
        return;

    m_transformMode = mode;
    emit transformModeChanged(m_transformMode, {});
}

void OpenGLWidget::handleTransformRotate(const int dx, PointRegistry &registry)
{
    const cadm::cadf angle = static_cast<cadm::cadf>(dx) * (2 * std::numbers::pi / static_cast<cadm::cadf>(
        width()));

    cadm::vec3 axis;
    if (m_xPressed)
        axis = cadm::vec3::unitX();
    else if (m_yPressed)
        axis = cadm::vec3::unitY();
    else if (m_zPressed)
        axis = cadm::vec3::unitZ();
    else
    {
        // Default: rotate around camera's view-forward direction
        const auto view = m_cameraController.getActiveStrategy()->getView();
        axis = -view.col(2).xyz().normalized();
    }

    const cadm::mat3 R = cadm::mat4::rotAxis(angle, axis).upperLeft3x3();
    for (const auto &snap : m_transformSnapshots)
    {
        const cadm::vec3 newPos = m_transformPivot + R * (snap.origPos - m_transformPivot);
        const auto entity = m_scene.getEntity(snap.id);
        if (!entity) continue;
        Entity *pEntity = entity.value();

        if (!snap.isTransformEntity)
        {
            if (const auto pc = pEntity->getComponent<PointComponent>())
                registry.setPosition(pc.value()->m_handle, newPos);
        }
        else
        {
            if (const auto tc = pEntity->getComponent<TransformComponent>())
            {
                tc.value()->setTranslation(newPos);
                tc.value()->setRotation(cadm::eulerZYXFromRotMat(R * snap.origRotMat));
            }
        }
    }
}

void OpenGLWidget::handleTransformTranslate(const QPoint currentMousePos, PointRegistry &registry)
{
    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto proj = m_cameraController.getActiveStrategy()->getProjection();
    const cadm::mat4 VP = proj * view;
    const cadm::mat4 invVP = view.inversedView() * m_cameraController.getActiveStrategy()->getInvProjection();

    const cadm::vec4 pivotClip = VP * cadm::vec4(m_transformPivot, 1);
    const cadm::cadf pivotNdcZ = pivotClip.z / pivotClip.w;
    auto unprojectAt = [&](const QPoint &p) -> cadm::vec3
    {
        return cadm::unprojectPoint({p.x(), p.y()}, pivotNdcZ, invVP, width(), height());
    };

    cadm::vec3 delta = unprojectAt(currentMousePos) - unprojectAt(m_transformStartMousePos);

    if (m_xPressed)
        delta = cadm::vec3::unitX() * cadm::vec3::unitX().dot(delta);
    else if (m_yPressed)
        delta = cadm::vec3::unitY() * cadm::vec3::unitY().dot(delta);
    else if (m_zPressed)
        delta = cadm::vec3::unitZ() * cadm::vec3::unitZ().dot(delta);

    for (const auto &snap : m_transformSnapshots)
    {
        const cadm::vec3 newPos = snap.origPos + delta;
        const auto entity = m_scene.getEntity(snap.id);
        if (!entity) continue;
        Entity *pEntity = entity.value();

        if (!snap.isTransformEntity)
        {
            if (const auto pc = pEntity->getComponent<PointComponent>())
                registry.setPosition(pc.value()->m_handle, newPos);
        }
        else
        {
            if (const auto tc = pEntity->getComponent<TransformComponent>())
                tc.value()->setTranslation(newPos);
        }
    }
}

void OpenGLWidget::handleTransformScale(const int dx, PointRegistry &registry)
{
    const cadm::cadf scaleFactor = std::exp(
        static_cast<cadm::cadf>(dx) * static_cast<cadm::cadf>(2.0) / static_cast<cadm::cadf>(width()));

    for (const auto &snap : m_transformSnapshots)
    {
        const cadm::vec3 newPos = m_transformPivot + (snap.origPos - m_transformPivot) * scaleFactor;
        const auto entity = m_scene.getEntity(snap.id);
        if (!entity) continue;
        Entity *pEntity = entity.value();

        if (!snap.isTransformEntity)
        {
            if (const auto pc = pEntity->getComponent<PointComponent>())
                registry.setPosition(pc.value()->m_handle, newPos);
        }
        else
        {
            if (const auto tc = pEntity->getComponent<TransformComponent>())
            {
                tc.value()->setTranslation(newPos);
                tc.value()->setScale(snap.origScale * scaleFactor);
            }
        }
    }
}

void OpenGLWidget::applyTransform(const QPoint currentMousePos)
{
    if (m_transformMode == TransformMode::None || m_transformSnapshots.empty())
        return;

    const int dx = currentMousePos.x() - m_transformStartMousePos.x();
    auto &registry = m_scene.getPointRegistry();

    switch (m_transformMode)
    {
    case TransformMode::None: // will never hit; just to silence the warning
        break;
    case TransformMode::Rotate:
        handleTransformRotate(dx, registry);
        break;
    case TransformMode::Scale:
        handleTransformScale(dx, registry);
        break;
    case TransformMode::Translate:
        handleTransformTranslate(currentMousePos, registry);
        break;
    }
}

void OpenGLWidget::cancelTransform()
{
    auto &registry = m_scene.getPointRegistry();
    for (const auto &snap : m_transformSnapshots)
    {
        const auto entity = m_scene.getEntity(snap.id);
        if (!entity) continue;
        snap.restoreEntity(registry, entity.value());
    }
    m_transformMode = TransformMode::None;
    m_transformSnapshots.clear();
    emit transformModeChanged(TransformMode::None, {});
}

void OpenGLWidget::confirmTransform()
{
    m_transformMode = TransformMode::None;
    m_transformSnapshots.clear();
    emit transformModeChanged(TransformMode::None, {});
    emit sceneChanged();
}

void OpenGLWidget::removeEntity(const EntityID id)
{
    m_cameraController.removeCamera(id);
    m_scene.removeEntity(id);
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
