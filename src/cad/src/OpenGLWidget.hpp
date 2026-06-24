//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_RENDERINGWINDOW_H
#define CAD_RENDERINGWINDOW_H

#include <QtOpenGLWidgets/QOpenGLWidget>

#include <cad_math/Common.hpp>
#include <cad_math/Vec3.hpp>
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
    none,
    cameraOrbit,
    cameraPan,
    cameraZoomDrag,
    boxSelect,
    clickSelect,
    cursorPlace,
    pointDrag,
};

class OpenGlWidget final : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OpenGlWidget(QWidget *parent = nullptr);

    ~OpenGlWidget() override;

    CameraController& getCameraController() {
        return m_cameraController;
    }

    bool removeEntity(EntityId id);

    bool eventFilter(QObject *obj, QEvent *event) override;

    [[nodiscard]] Scene& getScene() {
        return m_scene;
    }

    [[nodiscard]] CommandStack& getCommandStack() {
        return m_commandStack;
    }

    [[nodiscard]] const InputMap& getInputMap() const {
        return m_inputMap;
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

    void setInfiniteAxesMask(const int mask) {
        m_renderSystem.setInfiniteAxesMask(mask);
        update();
    }

    void setGridLodFade(const bool enabled) {
        m_renderSystem.setGridLodFade(enabled);
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

    void setStereoEnabled(const bool enabled) {
        m_stereoEnabled = enabled;
        update();
    }

    void setStereoAuto(const bool enabled) {
        m_stereoAuto = enabled;
        update();
    }

    void setStereoLuminance(const bool enabled) {
        m_stereoLuminance = enabled;
        update();
    }

    void setStereoAutoEyeSep(const bool enabled) {
        m_stereoAutoEyeSep = enabled;
        update();
    }

    /// @brief Separation = convergence / N
    void setStereoSeparationRatio(const double divisor) {
        m_stereoSeparationRatio = static_cast<cadm::cadf>(1.0 / divisor);
        update();
    }

    void setStereoEyeSeparation(const double sep) {
        const auto v = static_cast<cadm::cadf>(sep);
        if (std::abs(v - m_stereoEyeSeparation) < cadm::gc_eps) {
            return;
        }
        m_stereoEyeSeparation = v;
        emit stereoEyeSepChanged(sep);
        update();
    }

    void setStereoConvergence(const double dist) {
        const auto v = static_cast<cadm::cadf>(dist);
        if (std::abs(v - m_stereoConvergence) < cadm::gc_eps) {
            return;
        }
        m_stereoConvergence = v;
        emit stereoConvergenceChanged(dist);
        update();
    }

signals :
    void viewportSelectionChanged();

    /// Emitted when entities are added, removed, or renamed
    /// @note subscribe to this to sync entities (usually rebuild lists etc.)
    void sceneChanged();

    /// Emitted when geometry or properties change without structural scene changes
    /// (e.g., point drag in progress, cursor placement)
    /// @note subscribe to this to sync the properties of displayed object properties
    void geometryChanged();

    void stereoEyeSepChanged(double eyeSep);

    void stereoConvergenceChanged(double convergence);

    void transformModeChanged(TransformMode mode, QString axisInfo);

    void clickToAddModeChanged(bool active);

    void createTorusRequested();

    void createCursorRequested();

    void createPointRequested();

    void createBezierC0Requested();

    void createBezierC2Requested();

    void createInterpC2Requested();

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

    [[nodiscard]] std::optional<cadm::Vec3> computePivot() const;

    void handleTransformRotate(int dx, PointRegistry &registry);

    void handleTransformTranslate(QPoint currentMousePos, PointRegistry &registry);

    void handleTransformScale(int dx, PointRegistry &registry);

    static QString axisLabel(AxisConstraint constraint);

    /// @brief Save entity states and begin transform operation
    void beginTransform(TransformMode mode);

    /// @brief Update the entities based on the state saved at the beginning of the transform
    /// this way no numerical errors are accumulated
    void applyTransform(QPoint currentMousePos);

    /// @brief Restore the state of entities at the beginning of the transform operation
    void cancelTransform();

    /// @brief Confirm the transform; clear the states saved at the beginning of the transform
    void confirmTransform();

    static constexpr int s_clickRadiusPx = 8;

    void wrapMouseIfNeeded(QPoint currentPos, QPoint delta);

    bool removeEntityInternal(EntityId id);

    cadm::cadf m_sensitivity{0.001};
    cadm::cadf m_translationStep{0.1};
    QPoint m_lastMousePosition;
    QPoint m_pressPosition;

    CameraController m_cameraController{this};
    InputMap m_inputMap;

    /// @brief Holds currently active drag i.e., the drag that is taking place right now
    DragMode m_activeDrag{DragMode::none};
    cadm::cadf m_zoomFactor{1.1};

    Scene m_scene;
    CommandStack m_commandStack;
    RenderSystem m_renderSystem;

    bool m_boxSelectMode{false};

    /// @brief Guards against stacking multiple create menus
    bool m_createMenuOpen{false};
    QPoint m_boxSelectStart;
    QPoint m_boxSelectCurrent;

    std::unique_ptr<IViewportPositionStrategy> m_cursorPlacementStrategy;

    /// @brief Valid only in PointDrag mode
    PointHandle m_draggedPoint = 0;
    /// @brief Position of the dragged point when the drag began (for undo)
    cadm::Vec3 m_draggedPointStart;
    /// @brief Active cursor translation when a cursor-placement drag began (for undo)
    cadm::Vec3 m_cursorPlaceStart;

    /// @brief When true, LMB click places a new point at the cursor
    bool m_clickToAddMode = false;

    PivotMode m_pivotMode = PivotMode::medianPoint;
    CoordSpace m_coordSpace = CoordSpace::world;
    TransformMode m_transformMode = TransformMode::none;
    AxisConstraint m_axisConstraint = AxisConstraint::none;
    std::vector<EntitySnapshot> m_transformSnapshots;
    /// @brief Whether the active transform actually moved anything (gate undo entry)
    bool m_transformApplied = false;
    QPoint m_transformStartMousePos;
    cadm::Vec3 m_transformPivot;

    /// @brief Anaglyph stereoscopy state
    bool m_stereoEnabled = false;
    /// @brief Derive convergence/separation from camera distance to target each frame
    bool m_stereoAuto = true;
    /// @brief Use luminance anaglyph instead of channel split
    bool m_stereoLuminance = true;
    /// @brief Whether auto mode also drives eye distance
    bool m_stereoAutoEyeSep = true;
    /// @brief Eye separation as a fraction of convergence distance in auto mode (~1/30 comfort rule)
    cadm::cadf m_stereoSeparationRatio = 1.0 / 30.0;
    /// @brief Eye separation for stereoscopy in world units
    cadm::cadf m_stereoEyeSeparation = 0.3;
    /// @brief Projection-plane distance for stereoscopy in world units
    cadm::cadf m_stereoConvergence = 10.0;
};

#endif //CAD_RENDERINGWINDOW_H
