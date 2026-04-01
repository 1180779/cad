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
#include "cursor/ICursorPlacementStrategy.hpp"

enum class PivotMode { MedianPoint, ActiveCursor };

class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget() override;

    CameraController& getCameraController() { return m_cameraController; }

    bool eventFilter(QObject *obj, QEvent *event) override;

    Scene& getScene() { return m_scene; }

    void setPivotMode(PivotMode mode) { m_pivotMode = mode; }

    void setGridPlanes(const int planes)
    {
        m_renderSystem.setGridPlanes(planes);
        if (m_cursorPlacementStrategy)
            m_cursorPlacementStrategy->onGridPlanesChanged(planes);
        update();
    }

    void setCursorPlacementStrategy(std::shared_ptr<ICursorPlacementStrategy> strategy)
    {
        m_cursorPlacementStrategy = std::move(strategy);
    }

signals:
    void selectedEntityChanged(Entity *entity);
    void viewportSelectionChanged();
    void sceneChanged();

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

    [[nodiscard]] std::optional<cadm::vec3> computePivot() const;

    static constexpr int s_clickRadiusPx = 6;

    cadm::cadf m_sensitivity{0.001};
    cadm::cadf m_translationStep{0.1};
    QPoint m_lastMousePosition;
    QPoint m_pressPosition;

    CameraController m_cameraController{this};
    bool m_xPressed{false}, m_yPressed{false}, m_zPressed{false};
    cadm::cadf m_zoomFactor{1.1};

    Scene m_scene;
    RenderSystem m_renderSystem;

    bool m_boxSelectMode{false};
    bool m_boxSelecting{false};
    QPoint m_boxSelectStart;
    QPoint m_boxSelectCurrent;
    Qt::MouseButton m_boxSelectMouseButton = Qt::LeftButton;

    std::shared_ptr<ICursorPlacementStrategy> m_cursorPlacementStrategy;
    PivotMode m_pivotMode = PivotMode::MedianPoint;
};


#endif //CAD_RENDERINGWINDOW_H
