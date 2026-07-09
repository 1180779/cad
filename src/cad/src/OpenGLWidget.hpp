//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_RENDERINGWINDOW_H
#define CAD_RENDERINGWINDOW_H

#include <optional>
#include <utility>

#include <QtOpenGLWidgets/QOpenGLWidget>

#include <cad_math/Common.hpp>
#include <cad_math/Vec3.hpp>
#include "PatchGeometry.hxx"
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

class PatchComponent;
class OpenGlWidget;

namespace aliases {
    using GlW = OpenGlWidget;
}

class OpenGlWidget final : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OpenGlWidget(QWidget *parent = nullptr);

    ~OpenGlWidget() override;

    CameraController& getCameraController() {
        return m_cameraController;
    }

    bool removeEntity(EntityId id);

    /// @brief Collapse the two selected point entities into one (undoable);
    /// no-op unless exactly two points are selected
    void collapseSelectedPoints();

    bool eventFilter(QObject *obj, QEvent *event) override;

    [[nodiscard]] Scene& getScene() {
        return m_scene;
    }

    [[nodiscard]] CommandStack& getCommandStack() {
        return m_commandStack;
    }

    /// @brief Show/refresh a transient live preview of a patch about to be
    /// created (no scene/undo side effects)
    /// @note The preview follows the active cursor
    /// @warning When the grid topology changes, the rebuild wraps GL work in
    /// <code>makeCurrent()</code>/<code>doneCurrent()</code> and thus releases
    /// the current GL context. Same-topology refreshes (placement/dimension
    /// changes) are CPU-only and safe anywhere
    void setPatchPreview(const patchgen::PatchCreateParams &params);

    /// @brief Toggle the control-net wireframe of the live patch preview
    void setPatchPreviewShowNet(bool v);

    /// @brief Hide all scene entities except the active cursor (grid/axes stay)
    /// while the live patch preview is active, isolating the preview
    void setPatchPreviewHideScene(bool v);

    /// @brief Active cursor {translation, Euler ZYX rotation}, or identity
    /// placement without a cursor
    [[nodiscard]] std::pair<cadm::Vec3, cadm::Vec3> activeCursorPlacement() const;

    /// @brief Tear down the live patch preview
    void clearPatchPreview();

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

    void createPatchC0Requested();

    void createPatchC2Requested();

    void createGregoryRequested();

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
    static void clearBuffers(QOpenGLFunctions_4_5_Core *gl);

    void calculateStereoProjections(
        const cadm::Mat4 &view,
        const cadm::Mat4 &projection,
        std::span<cadm::Mat4, 2> views,
        std::span<cadm::Mat4, 2> projs
    ) const;

    void renderTransformAxis() const;

    void renderBoxSelectionRectangle() const;

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

    /// @brief Live preview of a patch being configured in the creation dialog:
    /// a self-contained point registry + patch component
    std::unique_ptr<PointRegistry> m_previewRegistry;
    std::unique_ptr<PatchComponent> m_previewPatch;
    std::optional<patchgen::PatchCreateParams> m_previewParams;
    bool m_previewShowNet{true};
    bool m_previewHideScene{false};

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
