//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.hpp"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QKeyEvent>
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
#include "commands/Commands.hpp"
#include "ViewportTypes.hpp"
#include "cad_math/Helpers.hpp"
#include "components/BezierC0Component.hpp"
#include "components/PointComponent.hpp"
#include "components/TransformComponent.hpp"
#include "cursor/GridPlanePlacementStrategy.hpp"

OpenGlWidget::OpenGlWidget(QWidget *parent) : QOpenGLWidget(parent),
                                              m_cursorPlacementStrategy(
                                                  std::make_unique<GridPlanePlacementStrategy>(1 /*XY plane*/)
                                              ) {
    setFocusPolicy(Qt::StrongFocus);
    // refresh the viewport and dependent panels after any undo/redo/push
    m_commandStack.onChange = [this] {
        m_scene.syncPointSelectionToRegistry();
        emit sceneChanged();
        emit viewportSelectionChanged();
        emit geometryChanged();
        update();
    };
}

OpenGlWidget::~OpenGlWidget() = default;

void OpenGlWidget::paintGL() {
    const auto gl = getGl();
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto projection = m_cameraController.getActiveStrategy()->getProjection();
    const auto invVp = view.inversedView() * m_cameraController.getActiveStrategy()->getInvProjection();
    m_renderSystem.render(m_scene, view, projection, invVp);

    if (const auto pivot = computePivot()) {
        m_renderSystem.renderPivotMarker(pivot.value(), view, projection);
    }

    if (m_transformMode != TransformMode::none) {
        int axesMask = 0;
        switch (m_axisConstraint) {
        case AxisConstraint::x:
            axesMask |= 1;
            break;
        case AxisConstraint::y:
            axesMask |= 2;
            break;
        case AxisConstraint::z:
            axesMask |= 4;
            break;
        default:
            break;
        }

        if (axesMask != 0) {
            cadm::Mat4 axisModel = cadm::Mat4::identity();
            if (m_coordSpace == CoordSpace::local && !m_transformSnapshots.empty()) {
                const auto &r = m_transformSnapshots[0].origRotMat;
                axisModel = cadm::Mat4{
                    cadm::vec4(r.columns[0], 0),
                    cadm::vec4(r.columns[1], 0),
                    cadm::vec4(r.columns[2], 0),
                    cadm::vec4::unitW()
                };
            }
            m_renderSystem.renderTransformAxis(m_transformPivot, axisModel, axesMask, view, projection, invVp);
        }
    }

    if (m_activeDrag == DragMode::boxSelect) {
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

void OpenGlWidget::resizeGL(const int width, const int height) {
    QOpenGLWidget::resizeGL(width, height);
    m_cameraController.getActiveStrategy()->syncAspectRatio();
    m_renderSystem.setViewport(width, height);
}

void OpenGlWidget::initializeGL() {
    const auto gl = getGl();
    gl->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glEnable(GL_PROGRAM_POINT_SIZE);

    m_renderSystem.initialize();
    m_renderSystem.setViewport(width(), height());
    m_scene.getPointRegistry().initialize();
}

void OpenGlWidget::mousePressEvent(QMouseEvent *event) {
    m_lastMousePosition = event->pos();
    m_pressPosition = event->pos();

    const auto action = m_inputMap.matchAction(event->button(), event->modifiers());
    if (!action) {
        return;
    }

    switch (*action) {
    case InputAction::cameraOrbit:
        m_activeDrag = DragMode::cameraOrbit;
        return;
    case InputAction::cameraPan:
        m_activeDrag = DragMode::cameraPan;
        return;
    case InputAction::cameraZoomDrag:
        m_activeDrag = DragMode::cameraZoomDrag;
        return;
    case InputAction::select:
        if (m_transformMode != TransformMode::none) {
            return;
        }
        if (m_clickToAddMode) {
            // resolve 3D position, move active cursor there, then emit createPointRequested
            if (m_cursorPlacementStrategy) {
                auto *cam = m_cameraController.getActiveStrategy();
                const auto invView = cam->getView().inversedView();
                const auto invProj = cam->getInvProjection();
                if (const auto pos = m_cursorPlacementStrategy->resolve(event, width(), height(), invView, invProj);
                    pos.has_value()) {
                    if (Entity *cursor = m_scene.getActiveCursor()) {
                        if (const auto tc = cursor->getComponent<TransformComponent>()) {
                            tc.value()->setTranslation(pos.value());
                        }
                    }
                    emit createPointRequested();
                }
            }
            return;
        }
        if (m_boxSelectMode) {
            m_activeDrag = DragMode::boxSelect;
            m_boxSelectStart = event->pos();
            m_boxSelectCurrent = event->pos();
        }
        else {
            // try to pick a control point for direct dragging
            if (const PointHandle hit = pickPoint(event->pos());
                hit != InvalidPointHandle) {
                m_draggedPoint = hit;
                m_draggedPointStart = m_scene.getPointRegistry().getPosition(hit);
                m_activeDrag = DragMode::pointDrag;
            }
            else {
                m_activeDrag = DragMode::clickSelect;
            }
        }
        return;
    case InputAction::cursorPlace:
        if (m_transformMode != TransformMode::none) {
            return;
        }
        m_activeDrag = DragMode::cursorPlace;
        if (Entity *cursor = m_scene.getActiveCursor()) {
            if (const auto tc = cursor->getComponent<TransformComponent>()) {
                m_cursorPlaceStart = tc.value()->getTranslation();
            }
        }
        break;
    default:
        break;
    }
}

void OpenGlWidget::mouseMoveEvent(QMouseEvent *event) {
    const auto currentPos = event->pos();

    const auto delta = currentPos - m_lastMousePosition;
    m_lastMousePosition = currentPos;

    if (m_transformMode != TransformMode::none) {
        applyTransform(currentPos);
        wrapMouseIfNeeded(currentPos, delta);
        update();
        return;
    }

    switch (m_activeDrag) {
    case DragMode::boxSelect:
        m_boxSelectCurrent = currentPos;
        update();
        return;
    case DragMode::pointDrag:
        if (m_cursorPlacementStrategy) {
            auto *cam = m_cameraController.getActiveStrategy();
            const auto invView = cam->getView().inversedView();
            const auto invProj = cam->getInvProjection();
            if (const auto pos = m_cursorPlacementStrategy->resolve(event, width(), height(), invView, invProj);
                pos.has_value()) {
                m_scene.getPointRegistry().setPosition(m_draggedPoint, pos.value());
                emit geometryChanged();
                update();
            }
        }
        return;
    case DragMode::cursorPlace:
        if (Entity *cursor = m_scene.getActiveCursor()) {
            auto *cam = m_cameraController.getActiveStrategy();
            const auto invView = cam->getView().inversedView();
            const auto invProj = cam->getInvProjection();
            if (const auto pos = m_cursorPlacementStrategy->resolve(event, width(), height(), invView, invProj);
                pos.has_value()) {
                if (const auto transform = cursor->getComponent<TransformComponent>()) {
                    transform.value()->setTranslation(pos.value());
                }
                emit sceneChanged();
                update();
            }
        }
        return;
    case DragMode::cameraOrbit:
        if (m_cameraController.getActiveStrategy()->handleCameraMove(CameraAction::orbit, delta)) {
            update();
        }
        wrapMouseIfNeeded(currentPos, delta);
        return;
    case DragMode::cameraPan:
        if (m_cameraController.getActiveStrategy()->handleCameraMove(CameraAction::pan, delta)) {
            update();
        }
        wrapMouseIfNeeded(currentPos, delta);
        return;
    case DragMode::cameraZoomDrag:
        if (m_cameraController.getActiveStrategy()->handleCameraMove(CameraAction::zoomDrag, delta)) {
            update();
        }
        wrapMouseIfNeeded(currentPos, delta);
    default:
        break;
    }
}

void OpenGlWidget::mouseReleaseEvent(QMouseEvent *event) {
    // a gizmo transform is in progress: release confirms it or cancels it;
    // no drag-mode handling applies
    if (m_transformMode != TransformMode::none) {
        const auto action = m_inputMap.matchAction(event->button(), event->modifiers());
        if (action == InputAction::select) {
            confirmTransform();
            update();
            return;
        }

        if (action == InputAction::rightClick) {
            cancelTransform();
            update();
            return;
        }
    }

    // otherwise finish whichever drag was active
    // (point drag/select, box select, click select, or cursor placement)

    const DragMode finishedDrag = m_activeDrag;
    m_activeDrag = DragMode::none;

    switch (finishedDrag) {
    case DragMode::pointDrag: {
        const auto diff = event->pos() - m_pressPosition;
        if (const auto dist2 = diff.x() * diff.x() + diff.y() * diff.y();
            dist2 <= s_clickRadiusPx * s_clickRadiusPx) {
            // treat as a selection click on the point
            const bool additive = m_inputMap.matchAction(event->button(), event->modifiers()) ==
                InputAction::cursorPlace;
            selectPoint(m_draggedPoint, additive);
        }
        else {
            // record the completed drag as a single undoable move
            const cadm::Vec3 after = m_scene.getPointRegistry().getPosition(m_draggedPoint);
            m_commandStack.push(
                std::make_unique<MovePointCommand>(m_scene, m_draggedPoint, m_draggedPointStart, after)
            );
            m_scene.getPointRegistry().syncToGpu();
            emit sceneChanged();
            update();
        }
        return;
    }

    case DragMode::boxSelect:
        m_boxSelectMode = false;
        performBoxSelect();
        update();
        return;

    case DragMode::clickSelect: {
        const auto diff = event->pos() - m_pressPosition;
        if (const auto dist2 = diff.x() * diff.x() + diff.y() * diff.y();
            dist2 > s_clickRadiusPx * s_clickRadiusPx) {
            return;
        }

        const bool additive = m_inputMap.matchAction(event->button(), event->modifiers()) ==
            InputAction::cursorPlace;

        if (const PointHandle hit = pickPoint(event->pos());
            hit != InvalidPointHandle) {
            selectPoint(hit, additive);
        }
        else if (!additive) // deselect all
        {
            m_scene.clearSelection();
            m_scene.syncPointSelectionToRegistry();
            emit viewportSelectionChanged();
            update();
        }
        return;
    }

    case DragMode::cursorPlace: {
        Entity *cursor = m_scene.getActiveCursor();
        if (!cursor) {
            return;
        }
        const auto tc = cursor->getComponent<TransformComponent>();
        if (!tc) {
            return;
        }
        const cadm::Vec3 before = m_cursorPlaceStart;
        const cadm::Vec3 after = tc.value()->getTranslation();
        if (before == after) {
            return;
        }

        const EntityId id = cursor->getId();
        Scene *scene = &m_scene;
        auto setCursor = [scene, id](const cadm::Vec3 p) {
            if (const auto e = scene->getEntity(id)) {
                if (const auto t = e.value()->getComponent<TransformComponent>()) {
                    t.value()->setTranslation(p);
                }
            }
        };
        m_commandStack.push(
            std::make_unique<SetPropertyCommand>(
                [setCursor, after] {
                    setCursor(after);
                },
                [setCursor, before] {
                    setCursor(before);
                },
                nullptr
            )
        );
    }

    default:
        break;
    }
}

void OpenGlWidget::wheelEvent(QWheelEvent *event) {
    if (m_cameraController.getActiveStrategy()->handleWheelEvent(event)) {
        update();
    }
}

void OpenGlWidget::keyPressEvent(QKeyEvent *event) {
    const auto action = m_inputMap.matchAction(
        static_cast<Qt::Key>(event->key()),
        event->modifiers(),
        event->isAutoRepeat()
    );
    if (!action) {
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

    auto toggleBeginTransform = [&](const TransformMode mode) {
        if (m_transformMode == mode) {
            cancelTransform();
        }
        else {
            if (m_transformMode != TransformMode::none) {
                confirmTransform();
            }
            beginTransform(mode);
        }
        update();
    };

    auto toggleConstraint = [&](const AxisConstraint axis) {
        if (m_transformMode == TransformMode::none) {
            return;
        }
        m_axisConstraint = m_axisConstraint == axis
                               ? AxisConstraint::none
                               : axis;
        emit transformModeChanged(m_transformMode, axisLabel(m_axisConstraint));
        update();
    };

    switch (*action) {
    case InputAction::cancelTransform:
        if (m_transformMode != TransformMode::none) {
            cancelTransform();
            update();
        }
        break;
    case InputAction::confirmTransform:
        if (m_transformMode != TransformMode::none) {
            confirmTransform();
            update();
        }
        break;
    case InputAction::beginTranslate:
        toggleBeginTransform(TransformMode::translate);
        break;
    case InputAction::beginRotate:
        toggleBeginTransform(TransformMode::rotate);
        break;
    case InputAction::beginScale:
        toggleBeginTransform(TransformMode::scale);
        break;
    case InputAction::constrainX:
        toggleConstraint(AxisConstraint::x);
        break;
    case InputAction::constrainY:
        toggleConstraint(AxisConstraint::y);
        break;
    case InputAction::constrainZ:
        toggleConstraint(AxisConstraint::z);
        break;
    case InputAction::toggleCoordSpace:
        m_coordSpace = m_coordSpace == CoordSpace::world
                           ? CoordSpace::local
                           : CoordSpace::world;
        update();
        break;
    case InputAction::switchCamera:
        m_cameraController.switchToNext();
        update();
        break;
    case InputAction::deleteSelected:
        deleteSelectedEntities();
        break;
    case InputAction::undo:
        m_commandStack.undo();
        break;
    case InputAction::redo:
        m_commandStack.redo();
        break;
    case InputAction::setBoxSelectMode:
        m_boxSelectMode = true;
        break;
    case InputAction::createMenu: {
        QMenu menu(this);
        menu.addAction(
            "New Torus",
            [this] {
                emit createTorusRequested();
            }
        );
        menu.addAction(
            "New Cursor",
            [this] {
                emit createCursorRequested();
            }
        );
        menu.addAction(
            "New Point",
            [this] {
                emit createPointRequested();
            }
        );
        menu.exec(QCursor::pos());
    }
    break;
    case InputAction::toggleClickToAdd:
        setClickToAddMode(!m_clickToAddMode);
        break;
    case InputAction::cameraToggleProjection:
        m_cameraController.getActiveStrategy()->toggleProjection();
        update();
        break;
    case InputAction::cameraMoveUp:
        if (m_cameraController.getActiveStrategy()->handleCameraKeyAction(CameraKeyAction::moveUp)) {
            update();
        }
        break;
    case InputAction::cameraMoveDown:
        if (m_cameraController.getActiveStrategy()->handleCameraKeyAction(CameraKeyAction::moveDown)) {
            update();
        }
        break;
    case InputAction::cameraMoveLeft:
        if (m_cameraController.getActiveStrategy()->handleCameraKeyAction(CameraKeyAction::moveLeft)) {
            update();
        }
        break;
    case InputAction::cameraMoveRight:
        if (m_cameraController.getActiveStrategy()->handleCameraKeyAction(CameraKeyAction::moveRight)) {
            update();
        }
        break;
    default:
        break;
    }
}

void OpenGlWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) {
        return;
    }

    const auto action = m_inputMap.matchAction(static_cast<Qt::Key>(event->key()), event->modifiers());
    if (!action) {
        QOpenGLWidget::keyReleaseEvent(event);
        return;
    }

    switch (*action) {
    case InputAction::setBoxSelectMode:
        m_boxSelectMode = false;
        break;
    default:
        QOpenGLWidget::keyReleaseEvent(event);
    }
}

void OpenGlWidget::deleteSelectedEntities() {
    const Entity *activeCursor = m_scene.getActiveCursor();
    std::vector<EntityId> toDelete;
    for (const auto *e : m_scene.getSelectedEntities()) {
        if (m_cameraController.isEntityManagedAsCamera(e->getId())) {
            continue;
        }
        if (e == activeCursor) {
            continue;
        }
        toDelete.push_back(e->getId());
    }

    if (!toDelete.empty()) {
        m_commandStack.push(std::make_unique<DeleteEntityCommand>(m_scene, toDelete));
        emit sceneChanged();
        emit viewportSelectionChanged();
        update();
    }
}

PointHandle OpenGlWidget::pickPoint(const QPoint screenPos) const {
    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto projection = m_cameraController.getActiveStrategy()->getProjection();
    const auto &registry = m_scene.getPointRegistry();

    PointHandle best = InvalidPointHandle;
    int bestDist = s_clickRadiusPx * s_clickRadiusPx + 1;

    for (const PointHandle h : registry.aliveHandles()) {
        const auto screen = cadm::projectToScreenGl(
            registry.getPosition(h),
            view,
            projection,
            width(),
            height()
        );
        if (!screen.has_value()) {
            continue;
        }

        const int dx = screen->x - screenPos.x();
        const int dy = screen->y - screenPos.y();
        if (const int dist = dx * dx + dy * dy;
            dist < bestDist) {
            bestDist = dist;
            best = h;
        }
    }

    return best;
}

void OpenGlWidget::selectPoint(const PointHandle hit, const bool additive) {
    if (!additive) {
        m_scene.clearSelection();
    }

    if (const auto entity = m_scene.getEntityByPointHandle(hit)) {
        Entity *pEntity = entity.value();
        m_scene.setSelected(
            pEntity,
            additive
                ? !pEntity->isSelected()
                : true
        );
        m_scene.syncPointSelectionToRegistry();
    }
    emit viewportSelectionChanged();
    update();
}

void OpenGlWidget::performBoxSelect() {
    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto projection = m_cameraController.getActiveStrategy()->getProjection();
    const QRect rect = QRect(m_boxSelectStart, m_boxSelectCurrent).normalized();

    m_scene.clearSelection();
    const auto &registry = m_scene.getPointRegistry();
    for (const auto &e : m_scene.getEntities()) {
        cadm::Vec3 worldPos;
        if (const auto pc = e->getComponent<PointComponent>()) {
            worldPos = registry.getPosition(pc.value()->m_handle);
        }
        else if (const auto transform = e->getComponent<TransformComponent>()) {
            worldPos = transform.value()->getTranslation();
        }
        else {
            continue;
        }

        if (const auto screen = cadm::projectToScreenGl(worldPos, view, projection, width(), height());
            screen.has_value() && rect.contains(screen->x, screen->y)) {
            m_scene.setSelected(e.get(), true);
        }
    }

    m_scene.syncPointSelectionToRegistry();
    emit viewportSelectionChanged();
}

std::optional<cadm::Vec3> OpenGlWidget::computePivot() const {
    if (m_pivotMode == PivotMode::activeCursor) {
        if (Entity *cursor = m_scene.getActiveCursor()) {
            if (const auto tc = cursor->getComponent<TransformComponent>()) {
                return tc.value()->getTranslation();
            }
        }
        return std::nullopt;
    }

    cadm::Vec3 sum{};
    int count = 0;
    const auto &registry = m_scene.getPointRegistry();
    for (const auto &e : m_scene.getEntities()) {
        if (!e->isSelected()) {
            continue;
        }
        if (const auto pc = e->getComponent<PointComponent>()) {
            sum = sum + registry.getPosition(pc.value()->m_handle);
            ++count;
        }
        else if (const auto tc = e->getComponent<TransformComponent>()) {
            sum = sum + tc.value()->getTranslation();
            ++count;
        }
        else if (const auto bc = e->getComponent<BezierC0Component>()) {
            for (const auto h : bc.value()->getControlPoints()) {
                sum = sum + registry.getPosition(h);
                ++count;
            }
        }
    }
    if (count == 0) {
        return std::nullopt;
    }
    return sum * (static_cast<cadm::cadf>(1.0) / static_cast<cadm::cadf>(count));
}

QString OpenGlWidget::axisLabel(const AxisConstraint constraint) {
    switch (constraint) {
    case AxisConstraint::x:
        return "X";
    case AxisConstraint::y:
        return "Y";
    case AxisConstraint::z:
        return "Z";
    default:
        return {};
    }
}

void OpenGlWidget::beginTransform(const TransformMode mode) {
    const auto pivot = computePivot();
    if (!pivot.has_value()) {
        return;
    }

    m_transformPivot = pivot.value();
    m_transformStartMousePos = mapFromGlobal(QCursor::pos());
    m_transformSnapshots.clear();

    const auto &registry = m_scene.getPointRegistry();
    for (const auto &e : m_scene.getEntities()) {
        if (!e->isSelected()) {
            continue;
        }

        // Bezier curve: move all its control points rather than the curve entity itself
        if (const auto bc = e->getComponent<BezierC0Component>()) {
            for (const auto h : bc.value()->getControlPoints()) {
                if (const auto ptEntity = m_scene.getEntityByPointHandle(h)) {
                    const EntityId ptId = ptEntity.value()->getId();
                    const bool already = std::ranges::any_of(
                        m_transformSnapshots,
                        [ptId](const EntitySnapshot &s) {
                            return s.id == ptId;
                        }
                    );
                    if (!already) {
                        EntitySnapshot snap;
                        snap.fillFromEntity(registry, ptEntity.value());
                        m_transformSnapshots.push_back(snap);
                    }
                }
            }
            continue;
        }

        EntitySnapshot snap;
        if (!snap.fillFromEntity(registry, e.get())) {
            continue;
        }

        m_transformSnapshots.push_back(snap);
    }

    if (m_transformSnapshots.empty()) {
        return;
    }

    m_transformMode = mode;
    m_transformApplied = false;
    emit transformModeChanged(m_transformMode, {});
}

void OpenGlWidget::handleTransformRotate(const int dx, PointRegistry &registry) {
    const auto angle = static_cast<cadm::cadf>(
        static_cast<cadm::cadf>(dx) * (2 * std::numbers::pi / static_cast<cadm::cadf>(width()))
    );

    for (const auto &snap : m_transformSnapshots) {
        const bool useLocal = m_coordSpace == CoordSpace::local && snap.isTransformEntity;

        cadm::Vec3 axis;
        switch (m_axisConstraint) {
        case AxisConstraint::x:
            axis = useLocal
                       ? snap.origRotMat.columns[0]
                       : cadm::Vec3::unitX();
            break;
        case AxisConstraint::y:
            axis = useLocal
                       ? snap.origRotMat.columns[1]
                       : cadm::Vec3::unitY();
            break;
        case AxisConstraint::z:
            axis = useLocal
                       ? snap.origRotMat.columns[2]
                       : cadm::Vec3::unitZ();
            break;
        default:
            // rotate around camera's view-forward direction
            const auto view = m_cameraController.getActiveStrategy()->getView();
            axis = -view.col(2).xyz().normalized();
            break;
        }

        const cadm::Vec3 pivot = useLocal
                                     ? snap.origPos
                                     : m_transformPivot;
        const cadm::Mat3 r = cadm::Mat4::rotAxis(angle, axis).upperLeft3X3();
        const cadm::Vec3 newPos = pivot + r * (snap.origPos - pivot);

        const auto entity = m_scene.getEntity(snap.id);
        if (!entity) {
            continue;
        }
        Entity *pEntity = entity.value();

        if (!snap.isTransformEntity) {
            if (const auto pc = pEntity->getComponent<PointComponent>()) {
                registry.setPosition(pc.value()->m_handle, newPos);
            }
        }
        else {
            if (const auto tc = pEntity->getComponent<TransformComponent>()) {
                tc.value()->setTranslation(newPos);
                tc.value()->setRotation(cadm::eulerZYXFromRotMat(r * snap.origRotMat));
            }
        }
    }
}

void OpenGlWidget::handleTransformTranslate(const QPoint currentMousePos, PointRegistry &registry) {
    const auto view = m_cameraController.getActiveStrategy()->getView();
    const auto proj = m_cameraController.getActiveStrategy()->getProjection();
    const cadm::Mat4 vp = proj * view;
    const cadm::Mat4 invVp = view.inversedView() * m_cameraController.getActiveStrategy()->getInvProjection();

    const cadm::vec4 pivotClip = vp * cadm::vec4(m_transformPivot, 1);
    const cadm::cadf pivotNdcZ = pivotClip.z / pivotClip.w;
    auto unprojectAt = [&](const QPoint &p) -> cadm::Vec3 {
        return cadm::unprojectPoint({p.x(), p.y()}, pivotNdcZ, invVp, width(), height());
    };

    const cadm::Vec3 rawDelta = unprojectAt(currentMousePos) - unprojectAt(m_transformStartMousePos);

    for (const auto &snap : m_transformSnapshots) {
        const bool useLocal = m_coordSpace == CoordSpace::local && snap.isTransformEntity;

        cadm::Vec3 delta = rawDelta;
        switch (m_axisConstraint) {
        case AxisConstraint::x: {
            const cadm::Vec3 ax = useLocal
                                      ? snap.origRotMat.columns[0]
                                      : cadm::Vec3::unitX();
            delta = ax * ax.dot(rawDelta);
            break;
        }
        case AxisConstraint::y: {
            const cadm::Vec3 ax = useLocal
                                      ? snap.origRotMat.columns[1]
                                      : cadm::Vec3::unitY();
            delta = ax * ax.dot(rawDelta);
            break;
        }
        case AxisConstraint::z: {
            const cadm::Vec3 ax = useLocal
                                      ? snap.origRotMat.columns[2]
                                      : cadm::Vec3::unitZ();
            delta = ax * ax.dot(rawDelta);
            break;
        }
        default:
            break;
        }

        const cadm::Vec3 newPos = snap.origPos + delta;
        const auto entity = m_scene.getEntity(snap.id);
        if (!entity) {
            continue;
        }
        Entity *pEntity = entity.value();

        if (!snap.isTransformEntity) {
            if (const auto pc = pEntity->getComponent<PointComponent>()) {
                registry.setPosition(pc.value()->m_handle, newPos);
            }
        }
        else {
            if (const auto tc = pEntity->getComponent<TransformComponent>()) {
                tc.value()->setTranslation(newPos);
            }
        }
    }
}

void OpenGlWidget::handleTransformScale(const int dx, PointRegistry &registry) {
    const cadm::cadf scaleFactor = std::exp(
        static_cast<cadm::cadf>(dx) * static_cast<cadm::cadf>(2.0) / static_cast<cadm::cadf>(width())
    );

    for (const auto &snap : m_transformSnapshots) {
        const cadm::Vec3 pivot = (m_coordSpace == CoordSpace::local && snap.isTransformEntity)
                                     ? snap.origPos
                                     : m_transformPivot;
        const cadm::Vec3 newPos = pivot + (snap.origPos - pivot) * scaleFactor;
        const auto entity = m_scene.getEntity(snap.id);
        if (!entity) {
            continue;
        }
        Entity *pEntity = entity.value();

        if (!snap.isTransformEntity) {
            if (const auto pc = pEntity->getComponent<PointComponent>()) {
                registry.setPosition(pc.value()->m_handle, newPos);
            }
        }
        else {
            if (const auto tc = pEntity->getComponent<TransformComponent>()) {
                tc.value()->setTranslation(newPos);
                tc.value()->setScale(snap.origScale * scaleFactor);
            }
        }
    }
}

void OpenGlWidget::applyTransform(const QPoint currentMousePos) {
    if (m_transformMode == TransformMode::none || m_transformSnapshots.empty()) {
        return;
    }

    m_transformApplied = true;
    const int dx = currentMousePos.x() - m_transformStartMousePos.x();
    auto &registry = m_scene.getPointRegistry();

    switch (m_transformMode) {
    case TransformMode::none: // will never hit; just to silence the warning
        break;
    case TransformMode::rotate:
        handleTransformRotate(dx, registry);
        break;
    case TransformMode::scale:
        handleTransformScale(dx, registry);
        break;
    case TransformMode::translate:
        handleTransformTranslate(currentMousePos, registry);
        break;
    }
}

void OpenGlWidget::cancelTransform() {
    auto &registry = m_scene.getPointRegistry();
    for (const auto &snap : m_transformSnapshots) {
        const auto entity = m_scene.getEntity(snap.id);
        if (!entity) {
            continue;
        }
        snap.restoreEntity(registry, entity.value());
    }
    m_transformMode = TransformMode::none;
    m_axisConstraint = AxisConstraint::none;
    m_transformSnapshots.clear();
    emit transformModeChanged(TransformMode::none, {});
    emit sceneChanged();
}

void OpenGlWidget::confirmTransform() {
    if (m_transformApplied && !m_transformSnapshots.empty()) {
        // capture the resulting state and record the gesture as one undo step
        // the change is already applied live, so the command's execute() is a no-op redo
        const auto &registry = m_scene.getPointRegistry();
        std::vector<EntitySnapshot> after;
        after.reserve(m_transformSnapshots.size());
        for (const auto &before : m_transformSnapshots) {
            EntitySnapshot snap;
            snap.id = before.id;
            if (const auto e = m_scene.getEntity(before.id)) {
                snap.fillFromEntity(registry, e.value());
            }
            after.push_back(snap);
        }
        m_commandStack.push(std::make_unique<TransformCommand>(m_scene, m_transformSnapshots, std::move(after)));
    }

    m_transformMode = TransformMode::none;
    m_axisConstraint = AxisConstraint::none;
    m_transformApplied = false;
    m_transformSnapshots.clear();
    emit transformModeChanged(TransformMode::none, {});
    emit sceneChanged();
}

void OpenGlWidget::wrapMouseIfNeeded(const QPoint currentPos, const QPoint delta) {
    constexpr int margin = 2;
    const int w = width();
    const int h = height();

    QPoint newPos = currentPos;

    const auto rightBoundary = w - margin - 1;
    constexpr auto leftBoundary = margin;
    constexpr auto upBoundary = margin;
    const auto downBoundary = h - margin - 1;
    if (currentPos.x() <= leftBoundary && delta.x() < 0) {
        newPos.setX(rightBoundary);
    }
    else if (currentPos.x() >= rightBoundary && delta.x() > 0) {
        newPos.setX(leftBoundary);
    }

    if (currentPos.y() <= upBoundary && delta.y() < 0) {
        newPos.setY(downBoundary);
    }
    else if (currentPos.y() >= downBoundary && delta.y() > 0) {
        newPos.setY(upBoundary);
    }

    if (newPos == currentPos) {
        return;
    }

    if (m_transformMode != TransformMode::none) {
        const QPoint wrapDelta = newPos - currentPos;
        m_transformStartMousePos += wrapDelta;
    }

    m_lastMousePosition = newPos;
    QCursor::setPos(mapToGlobal(newPos));
}

bool OpenGlWidget::removeEntityInternal(const EntityId id) {
    m_cameraController.removeCamera(id);
    return m_scene.removeEntity(id);
}

bool OpenGlWidget::removeEntity(const EntityId id) {
    const auto entity = m_scene.getEntity(id);
    if (!entity) {
        return false;
    }

    // undoable for serializable geometry;
    // cameras/other entities fall back to the direct path
    // (which also detaches the camera controller)
    // and are not recorded
    if (EntitySpec probe;
        captureEntity(m_scene, entity.value(), probe)) {
        m_commandStack.push(std::make_unique<DeleteEntityCommand>(m_scene, std::vector{id}));
    }
    else {
        removeEntityInternal(id);
    }

    emit sceneChanged();
    emit viewportSelectionChanged();
    return true;
}

bool OpenGlWidget::eventFilter(QObject *obj, QEvent *event) {
    if (const QWidget *focus = QApplication::focusWidget();
        focus && focus != this) {
        return QObject::eventFilter(obj, event);
    }

    if (event->type() == QEvent::KeyPress) {
        const auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        keyPressEvent(keyEvent);
        return true;
    }
    if (event->type() == QEvent::KeyRelease) {
        const auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        keyReleaseEvent(keyEvent);
        return true;
    }
    return QObject::eventFilter(obj, event);
}
