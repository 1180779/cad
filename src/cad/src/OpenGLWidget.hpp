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
#include "camera/CameraController.hpp"
#include "components/EntitySnapshot.hpp"
#include "cursor/ICursorPlacementStrategy.hpp"
#include "input/InputMap.hpp"

enum class PivotMode { MedianPoint, ActiveCursor };

enum class TransformMode { None, Rotate, Scale, Translate };

enum class CoordSpace { World, Local };

enum class AxisConstraint { None, X, Y, Z };

enum class DragMode
{
    None,
    CameraOrbit,
    CameraPan,
    CameraZoomDrag,
    BoxSelect,
    ClickSelect,
    CursorPlace,
};

class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget() override;

    CameraController& getCameraController() { return m_cameraController; }

    void removeEntity(EntityID id);

    bool eventFilter(QObject *obj, QEvent *event) override;

    [[nodiscard]] Scene& getScene() { return m_scene; }
    [[nodiscard]] CoordSpace getCoordSpace() const { return m_coordSpace; }

    void setPivotMode(const PivotMode mode) { m_pivotMode = mode; }
    void setCoordSpace(const CoordSpace space) { m_coordSpace = space; }

    void setGridPlanes(const int planes)
    {
        m_renderSystem.setGridPlanes(planes);
        if (m_cursorPlacementStrategy)
            m_cursorPlacementStrategy->onGridPlanesChanged(planes);
        update();
    }

    void setCursorPlacementStrategy(std::unique_ptr<ICursorPlacementStrategy> strategy)
    {
        m_cursorPlacementStrategy = std::move(strategy);
    }

signals:
    void selectedEntityChanged(Entity *entity);
    void viewportSelectionChanged();
    void sceneChanged();
    void transformModeChanged(TransformMode mode, QString axisInfo);

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

    // save entities states and begin transform operation
    void beginTransform(TransformMode mode);

    // update the entities based on the state saved at the beginning of the transform
    // this way no numerical errors are accumulated
    void applyTransform(QPoint currentMousePos);

    // restore the state of entities at the beginning of the transform operation
    void cancelTransform();

    // confirm the transform; clear the states saved at the beginning of the transform
    void confirmTransform();

    static constexpr int s_clickRadiusPx = 8;

    void wrapMouseIfNeeded(QPoint currentPos, QPoint delta);

    cadm::cadf m_sensitivity{0.001};
    cadm::cadf m_translationStep{0.1};
    QPoint m_lastMousePosition;
    QPoint m_pressPosition;

    CameraController m_cameraController{this};
    InputMap m_inputMap;

    // holds currently active drag i.e. the drag that is taking place right now
    DragMode m_activeDrag{DragMode::None};
    cadm::cadf m_zoomFactor{1.1};

    Scene m_scene;
    RenderSystem m_renderSystem;

    bool m_boxSelectMode{false};
    QPoint m_boxSelectStart;
    QPoint m_boxSelectCurrent;

    std::unique_ptr<ICursorPlacementStrategy> m_cursorPlacementStrategy;

    PivotMode m_pivotMode = PivotMode::MedianPoint;
    CoordSpace m_coordSpace = CoordSpace::World;
    TransformMode m_transformMode = TransformMode::None;
    AxisConstraint m_axisConstraint = AxisConstraint::None;
    std::vector<EntitySnapshot> m_transformSnapshots;
    QPoint m_transformStartMousePos;
    cadm::vec3 m_transformPivot;
};


#endif //CAD_RENDERINGWINDOW_H
