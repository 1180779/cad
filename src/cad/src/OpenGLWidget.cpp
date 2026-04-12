//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.hpp"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QLineEdit>
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

    if (m_transformMode != TransformMode::None)
    {
        int axesMask = 0;
        switch (m_axisConstraint)
        {
        case AxisConstraint::X: axesMask |= 1;
            break;
        case AxisConstraint::Y: axesMask |= 2;
            break;
        case AxisConstraint::Z: axesMask |= 4;
            break;
        default: break;
        }

        if (axesMask != 0)
        {
            cadm::mat4 axisModel = cadm::mat4::identity();
            if (m_coordSpace == CoordSpace::Local && !m_transformSnapshots.empty())
            {
                const auto &r = m_transformSnapshots[0].origRotMat;
                axisModel = cadm::mat4{
                    cadm::vec4(r.columns[0], 0),
                    cadm::vec4(r.columns[1], 0),
                    cadm::vec4(r.columns[2], 0),
                    cadm::vec4::unitW()
                };
            }
            m_renderSystem.renderTransformAxis(m_transformPivot, axisModel, axesMask, view, projection, invVP);
        }
    }

    if (m_activeDrag == DragMode::BoxSelect)
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
    m_renderSystem.setViewport(width, height);
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
    m_renderSystem.setViewport(width(), height());
}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePosition = event->pos();
    m_pressPosition = event->pos();

    const auto action = m_inputMap.matchAction(event->button(), event->modifiers());
    if (!action)
        return;

    switch (*action)
    {
    case InputAction::CameraOrbit:
        m_activeDrag = DragMode::CameraOrbit;
        return;
    case InputAction::CameraPan:
        m_activeDrag = DragMode::CameraPan;
        return;
    case InputAction::CameraZoomDrag:
        m_activeDrag = DragMode::CameraZoomDrag;
        return;
    case InputAction::Select:
        if (m_transformMode != TransformMode::None)
            return;
        if (m_boxSelectMode)
        {
            m_activeDrag = DragMode::BoxSelect;
            m_boxSelectStart = event->pos();
            m_boxSelectCurrent = event->pos();
        }
        else
        {
            m_activeDrag = DragMode::ClickSelect;
        }
        return;
    case InputAction::CursorPlace:
        if (m_transformMode != TransformMode::None)
            return;
        m_activeDrag = DragMode::CursorPlace;
    default: break;
    }
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    const auto currentPos = event->pos();

    const auto delta = currentPos - m_lastMousePosition;
    m_lastMousePosition = currentPos;

    if (m_transformMode != TransformMode::None)
    {
        applyTransform(currentPos);
        wrapMouseIfNeeded(currentPos, delta);
        update();
        return;
    }

    switch (m_activeDrag)
    {
    case DragMode::BoxSelect:
        m_boxSelectCurrent = currentPos;
        update();
        return;
    case DragMode::CursorPlace:
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
    case DragMode::CameraOrbit:
        if (m_cameraController.getActiveStrategy()->handleCameraMove(CameraAction::Orbit, delta))
            update();
        wrapMouseIfNeeded(currentPos, delta);
        return;
    case DragMode::CameraPan:
        if (m_cameraController.getActiveStrategy()->handleCameraMove(CameraAction::Pan, delta))
            update();
        wrapMouseIfNeeded(currentPos, delta);
        return;
    case DragMode::CameraZoomDrag:
        if (m_cameraController.getActiveStrategy()->handleCameraMove(CameraAction::ZoomDrag, delta))
            update();
        wrapMouseIfNeeded(currentPos, delta);
    default: break;
    }
}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_transformMode != TransformMode::None)
    {
        const auto action = m_inputMap.matchAction(event->button(), event->modifiers());
        if (action == InputAction::Select)
        {
            confirmTransform();
            update();
            return;
        }

        if (action == InputAction::RightClick)
        {
            cancelTransform();
            update();
            return;
        }
    }

    const DragMode finishedDrag = m_activeDrag;
    m_activeDrag = DragMode::None;

    switch (finishedDrag)
    {
    case DragMode::BoxSelect:
        m_boxSelectMode = false;
        performBoxSelect();
        update();
        return;

    case DragMode::ClickSelect:
        {
            const auto diff = event->pos() - m_pressPosition;
            if (const auto dist2 = diff.x() * diff.x() + diff.y() * diff.y();
                dist2 > s_clickRadiusPx * s_clickRadiusPx)
                return;

            const bool additive = m_inputMap.matchAction(event->button(), event->modifiers()) ==
                InputAction::CursorPlace;

            if (const PointHandle hit = pickPoint(event->pos());
                hit != InvalidPointHandle)
            {
                selectPoint(hit, additive);
            }
            else if (!additive) // deselect all
            {
                // TODO: refactor to not go over all entities
                for (auto &e : m_scene.getEntities())
                    e->setSelected(false);
                m_scene.syncPointSelectionToRegistry();
                emit selectedEntityChanged(nullptr);
                emit viewportSelectionChanged();
                update();
            }
        }

    default: break;
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
    const auto action = m_inputMap.matchAction(
        static_cast<Qt::Key>(event->key()),
        event->modifiers(),
        event->isAutoRepeat());
    if (!action)
    {
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

    auto toggleBeginTransform = [&](const TransformMode mode)
    {
        if (m_transformMode == mode)
            cancelTransform();
        else
        {
            if (m_transformMode != TransformMode::None)
                confirmTransform();
            beginTransform(mode);
        }
        update();
    };

    auto toggleConstraint = [&](const AxisConstraint axis)
    {
        if (m_transformMode == TransformMode::None)
            return;
        m_axisConstraint = (m_axisConstraint == axis)
                               ? AxisConstraint::None
                               : axis;
        emit transformModeChanged(m_transformMode, axisLabel(m_axisConstraint));
        update();
    };

    switch (*action)
    {
    case InputAction::CancelTransform:
        if (m_transformMode != TransformMode::None)
        {
            cancelTransform();
            update();
        }
        break;
    case InputAction::ConfirmTransform:
        if (m_transformMode != TransformMode::None)
        {
            confirmTransform();
            update();
        }
        break;
    case InputAction::BeginTranslate:
        toggleBeginTransform(TransformMode::Translate);
        break;
    case InputAction::BeginRotate:
        toggleBeginTransform(TransformMode::Rotate);
        break;
    case InputAction::BeginScale:
        toggleBeginTransform(TransformMode::Scale);
        break;
    case InputAction::ConstrainX:
        toggleConstraint(AxisConstraint::X);
        break;
    case InputAction::ConstrainY:
        toggleConstraint(AxisConstraint::Y);
        break;
    case InputAction::ConstrainZ:
        toggleConstraint(AxisConstraint::Z);
        break;
    case InputAction::ToggleCoordSpace:
        m_coordSpace = (m_coordSpace == CoordSpace::World)
                           ? CoordSpace::Local
                           : CoordSpace::World;
        update();
        break;
    case InputAction::SwitchCamera:
        m_cameraController.switchToNext(width(), height());
        update();
        break;
    case InputAction::DeleteSelected:
        deleteSelectedEntities();
        break;
    case InputAction::SetBoxSelectMode:
        m_boxSelectMode = true;
        break;
    case InputAction::CreateMenu:
        {
            QMenu menu(this);
            menu.addAction("New Torus", [this] { emit createTorusRequested(); });
            menu.addAction("New Cursor", [this] { emit createCursorRequested(); });
            menu.addAction("New Point", [this] { emit createPointRequested(); });
            menu.exec(QCursor::pos());
        }
        break;
    case InputAction::CameraToggleProjection:
        m_cameraController.getActiveStrategy()->toggleProjection();
        update();
        break;
    case InputAction::CameraMoveUp:
        if (m_cameraController.getActiveStrategy()->handleCameraKeyAction(CameraKeyAction::MoveUp))
            update();
        break;
    case InputAction::CameraMoveDown:
        if (m_cameraController.getActiveStrategy()->handleCameraKeyAction(CameraKeyAction::MoveDown))
            update();
        break;
    case InputAction::CameraMoveLeft:
        if (m_cameraController.getActiveStrategy()->handleCameraKeyAction(CameraKeyAction::MoveLeft))
            update();
        break;
    case InputAction::CameraMoveRight:
        if (m_cameraController.getActiveStrategy()->handleCameraKeyAction(CameraKeyAction::MoveRight))
            update();
        break;
    default:
        break;
    }
}

void OpenGLWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;

    const auto action = m_inputMap.matchAction(static_cast<Qt::Key>(event->key()), event->modifiers());
    if (!action)
    {
        QOpenGLWidget::keyReleaseEvent(event);
        return;
    }

    switch (*action)
    {
    case InputAction::SetBoxSelectMode:
        m_boxSelectMode = false;
        break;
    default:
        QOpenGLWidget::keyReleaseEvent(event);
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
        if (const int dist = dx * dx + dy * dy;
            dist < bestDist)
        {
            bestDist = dist;
            best = h;
        }
    }

    return best;
}

void OpenGLWidget::selectPoint(const PointHandle hit, const bool additive)
{
    if (!additive)
    {
        // TODO: change this to not go through all the entities;
        //  and only do this after additives change
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

        // TODO: refactor this to not go though all entities each time
        // Only show properties when exactly one point entity is selected.
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
    if (count == 0) return std::nullopt;
    return sum * (static_cast<cadm::cadf>(1.0) / static_cast<cadm::cadf>(count));
}

QString OpenGLWidget::axisLabel(const AxisConstraint constraint)
{
    switch (constraint)
    {
    case AxisConstraint::X: return "X";
    case AxisConstraint::Y: return "Y";
    case AxisConstraint::Z: return "Z";
    default: return {};
    }
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
    const auto angle = static_cast<cadm::cadf>(
        static_cast<cadm::cadf>(dx) * (2 * std::numbers::pi / static_cast<cadm::cadf>(width()))
    );

    for (const auto &snap : m_transformSnapshots)
    {
        const bool useLocal = m_coordSpace == CoordSpace::Local && snap.isTransformEntity;

        cadm::vec3 axis;
        switch (m_axisConstraint)
        {
        case AxisConstraint::X:
            axis = useLocal
                       ? snap.origRotMat.columns[0]
                       : cadm::vec3::unitX();
            break;
        case AxisConstraint::Y:
            axis = useLocal
                       ? snap.origRotMat.columns[1]
                       : cadm::vec3::unitY();
            break;
        case AxisConstraint::Z:
            axis = useLocal
                       ? snap.origRotMat.columns[2]
                       : cadm::vec3::unitZ();
            break;
        default:
            // rotate around camera's view-forward direction
            const auto view = m_cameraController.getActiveStrategy()->getView();
            axis = -view.col(2).xyz().normalized();
            break;
        }

        const cadm::vec3 pivot = useLocal
                                     ? snap.origPos
                                     : m_transformPivot;
        const cadm::mat3 R = cadm::mat4::rotAxis(angle, axis).upperLeft3x3();
        const cadm::vec3 newPos = pivot + R * (snap.origPos - pivot);

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

    const cadm::vec3 rawDelta = unprojectAt(currentMousePos) - unprojectAt(m_transformStartMousePos);

    for (const auto &snap : m_transformSnapshots)
    {
        const bool useLocal = m_coordSpace == CoordSpace::Local && snap.isTransformEntity;

        cadm::vec3 delta = rawDelta;
        switch (m_axisConstraint)
        {
        case AxisConstraint::X:
            {
                const cadm::vec3 ax = useLocal
                                          ? snap.origRotMat.columns[0]
                                          : cadm::vec3::unitX();
                delta = ax * ax.dot(rawDelta);
                break;
            }
        case AxisConstraint::Y:
            {
                const cadm::vec3 ax = useLocal
                                          ? snap.origRotMat.columns[1]
                                          : cadm::vec3::unitY();
                delta = ax * ax.dot(rawDelta);
                break;
            }
        case AxisConstraint::Z:
            {
                const cadm::vec3 ax = useLocal
                                          ? snap.origRotMat.columns[2]
                                          : cadm::vec3::unitZ();
                delta = ax * ax.dot(rawDelta);
                break;
            }
        default: break;
        }

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
        const cadm::vec3 pivot = (m_coordSpace == CoordSpace::Local && snap.isTransformEntity)
                                     ? snap.origPos
                                     : m_transformPivot;
        const cadm::vec3 newPos = pivot + (snap.origPos - pivot) * scaleFactor;
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
    m_axisConstraint = AxisConstraint::None;
    m_transformSnapshots.clear();
    emit transformModeChanged(TransformMode::None, {});
}

void OpenGLWidget::confirmTransform()
{
    m_transformMode = TransformMode::None;
    m_axisConstraint = AxisConstraint::None;
    m_transformSnapshots.clear();
    emit transformModeChanged(TransformMode::None, {});
    emit sceneChanged();
}

void OpenGLWidget::wrapMouseIfNeeded(const QPoint currentPos, const QPoint delta)
{
    constexpr int margin = 2;
    const int w = width();
    const int h = height();

    QPoint newPos = currentPos;

    const auto rightBoundary = w - margin - 1;
    constexpr auto leftBoundary = margin;
    constexpr auto upBoundary = margin;
    const auto downBoundary = h - margin - 1;
    if (currentPos.x() <= leftBoundary && delta.x() < 0)
        newPos.setX(rightBoundary);
    else if (currentPos.x() >= rightBoundary && delta.x() > 0)
        newPos.setX(leftBoundary);

    if (currentPos.y() <= upBoundary && delta.y() < 0)
        newPos.setY(downBoundary);
    else if (currentPos.y() >= downBoundary && delta.y() > 0)
        newPos.setY(upBoundary);

    if (newPos == currentPos)
        return;

    if (m_transformMode != TransformMode::None)
    {
        const QPoint wrapDelta = newPos - currentPos;
        m_transformStartMousePos += wrapDelta;
    }

    m_lastMousePosition = newPos;
    QCursor::setPos(mapToGlobal(newPos));
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
