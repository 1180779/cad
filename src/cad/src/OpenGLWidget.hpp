//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_RENDERINGWINDOW_H
#define CAD_RENDERINGWINDOW_H

#include <QtOpenGLWidgets/QOpenGLWidget>

#include <cad_math/common.hpp>
#include <cad_math/vec3.hpp>
#include "PointRegistry.hpp"
#include "RenderSystem.hpp"
#include "Scene.hpp"
#include "ViewportTypes.hpp"
#include "camera/CameraController.hpp"
#include "commands/CommandStack.hpp"
#include "components/EntitySnapshot.hpp"
#include "cursor/ICursorPlacementStrategy.hpp"
#include "input/InputMap.hpp"

enum class DragMode {
    None,
    CameraOrbit,
    CameraPan,
    CameraZoomDrag,
    BoxSelect,
    ClickSelect,
    CursorPlace,
    PointDrag,
};

class OpenGLWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);

    ~OpenGLWidget() override;

    CameraController& getCameraController() {
        return m_cameraController;
    }

    bool removeEntity(EntityID id);

    bool eventFilter(QObject *obj, QEvent *event) override;

    [[nodiscard]] Scene& getScene() {
        return m_scene;
    }

    [[nodiscard]] CommandStack& getCommandStack() {
        return m_commandStack;
    }

    [[nodiscard]] CoordSpace getCoordSpace() const {
        return m_coordSpace;
    }

    void setPivotMode(const PivotMode mode) {
        m_pivotMode = mode;
    }

    void setCoordSpace(const CoordSpace space) {
        m_coordSpace = space;
    }

    void setGridPlanes(const int planes) {
        m_renderSystem.setGridPlanes(planes);
        if (m_cursorPlacementStrategy) {
            m_cursorPlacementStrategy->onGridPlanesChanged(planes);
        }
        update();
    }

    void setCursorPlacementStrategy(std::unique_ptr<IViewportPositionStrategy> strategy) {
        m_cursorPlacementStrategy = std::move(strategy);
    }

    [[nodiscard]] bool isClickToAddMode() const {
        return m_clickToAddMode;
    }

    void setClickToAddMode(const bool active) {
        m_clickToAddMode = active;
        emit clickToAddModeChanged(active);
    }

signals:
    void viewportSelectionChanged();

    /// Emitted when entities are added, removed, or renamed
    /// @note subscribe to this to sync entities (usually rebuild lists etc.)
    void sceneChanged();

    /// Emitted when geometry or properties change without structural scene changes
    /// (e.g., point drag in progress, cursor placement)
    /// @note subscribe to this to sync the properties of displayed object properties
    void geometryChanged();

    void transformModeChanged(TransformMode mode, QString axisInfo);

    void clickToAddModeChanged(bool active);

    void createTorusRequested();

    void createCursorRequested();

    void createPointRequested();

protected:
    void paintGL() override;

    void resizeGL(int width, int height) override;

    void initializeGL() override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void performBoxSelect();

    void deleteSelectedEntities();

    PointHandle pickPoint(QPoint screenPos) const;

    void selectPoint(PointHandle hit, bool additive);

    [[nodiscard]] std::optional<cadm::vec3> computePivot() const;

    void handleTransformRotate(int dx, PointRegistry &registry);

    void handleTransformTranslate(QPoint currentMousePos, PointRegistry &registry);

    void handleTransformScale(int dx, PointRegistry &registry);

    static QString axisLabel(AxisConstraint constraint);

    /// save entity states and begin transform operation
    void beginTransform(TransformMode mode);

    /// update the entities based on the state saved at the beginning of the transform
    /// this way no numerical errors are accumulated
    void applyTransform(QPoint currentMousePos);

    /// restore the state of entities at the beginning of the transform operation
    void cancelTransform();

    /// confirm the transform; clear the states saved at the beginning of the transform
    void confirmTransform();

    static constexpr int s_clickRadiusPx = 8;

    void wrapMouseIfNeeded(QPoint currentPos, QPoint delta);

    bool removeEntityInternal(EntityID id);

    cadm::cadf m_sensitivity{0.001};
    cadm::cadf m_translationStep{0.1};
    QPoint m_lastMousePosition;
    QPoint m_pressPosition;

    CameraController m_cameraController{this};
    InputMap m_inputMap;

    /// holds currently active drag i.e., the drag that is taking place right now
    DragMode m_activeDrag{DragMode::None};
    cadm::cadf m_zoomFactor{1.1};

    Scene m_scene;
    CommandStack m_commandStack;
    RenderSystem m_renderSystem;

    bool m_boxSelectMode{false};
    QPoint m_boxSelectStart;
    QPoint m_boxSelectCurrent;

    std::unique_ptr<IViewportPositionStrategy> m_cursorPlacementStrategy;

    /// valid only in PointDrag mode
    PointHandle m_draggedPoint = 0;
    /// position of the dragged point when the drag began (for undo)
    cadm::vec3 m_draggedPointStart;
    /// active cursor translation when a cursor-placement drag began (for undo)
    cadm::vec3 m_cursorPlaceStart;

    /// when true, LMB click places a new point at the cursor
    bool m_clickToAddMode = false;

    PivotMode m_pivotMode = PivotMode::medianPoint;
    CoordSpace m_coordSpace = CoordSpace::world;
    TransformMode m_transformMode = TransformMode::none;
    AxisConstraint m_axisConstraint = AxisConstraint::none;
    std::vector<EntitySnapshot> m_transformSnapshots;
    /// whether the active transform actually moved anything (gate undo entry)
    bool m_transformApplied = false;
    QPoint m_transformStartMousePos;
    cadm::vec3 m_transformPivot;
};

#endif //CAD_RENDERINGWINDOW_H
