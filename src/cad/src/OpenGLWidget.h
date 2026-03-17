//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_RENDERINGWINDOW_H
#define CAD_RENDERINGWINDOW_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QKeyEvent>

#include <cad_math/common.h>
#include "renderSystem.h"
#include "scene.h"

class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget() override;

    void paintGL() override;
    void resizeGL(int width, int height) override;
    void initializeGL() override;

    bool eventFilter(QObject *obj, QEvent *event) override;

    Scene& getScene() { return m_scene; }

signals:
    void selectedEntityChanged(entity *entity);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    cadm::cadf m_sensitivity{0.001};
    cadm::cadf m_translationStep{0.1};
    QPoint m_lastMousePosition;

    entity *m_mainCamera{nullptr};
    bool m_xPressed{false}, m_yPressed{false}, m_zPressed{false};
    cadm::cadf m_zoomFactor{1.1};

    Scene m_scene;
    RenderSystem m_renderSystem;
};


#endif //CAD_RENDERINGWINDOW_H