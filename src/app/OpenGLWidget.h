//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_RENDERINGWINDOW_H
#define CAD_RENDERINGWINDOW_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QKeyEvent>
#include <memory>

#include <cad_math/common.h>
#include "GL.h"
#include "ShaderProgram.h"
#include "quad.h"
#include "cad_math/vec3.h"
#include "cad_math/vec3i.h"
#include "Camera.h"

struct RenderState {
    int width = 0;
    int height = 0;
    cadm::mat4 invPV;
    cadm::mat4 Minv;
    cadm::mat4 MinvT;
    cadm::mat4 Dprim;
    cadm::vec3 cameraPos;
    cadm::vec3 specularColor;
    cadm::vec3i ambient;
    cadm::cadf m;
    cadm::cadf a, b, c;
    unsigned char adaptationSize;
};

class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget* parent = nullptr);
    ~OpenGLWidget() override;

    void paintGL() override;
    void resizeGL(int width, int height) override;
    void initializeGL() override;

    [[nodiscard]] cadm::cadf getA() const { return m_a; }
    [[nodiscard]] cadm::cadf getB() const { return m_b; }
    [[nodiscard]] cadm::cadf getC() const { return m_c; }
    void setA(cadm::cadf a);
    void setB(cadm::cadf b);
    void setC(cadm::cadf c);

    [[nodiscard]] cadm::vec3 getTranslation() const { return m_translation; }
    void setTranslation(cadm::vec3 translation);

    [[nodiscard]] cadm::vec3 getRotation() const { return m_rotation; }
    void setRotation(cadm::vec3 rotation);

    [[nodiscard]] cadm::vec3 getScale() const { return m_scale; }
    void setScale(cadm::vec3 scale);

    [[nodiscard]] unsigned char getAdaptationSize() const { return m_adaptationSize; }
    void setAdaptationSize(unsigned char adaptationSize);

    [[nodiscard]] cadm::cadf getM() const { return m_m; }
    void setM(cadm::cadf m);

    [[nodiscard]] int getAmbientR() const { return m_ambient.r; }
    [[nodiscard]] int getAmbientG() const { return m_ambient.g; }
    [[nodiscard]] int getAmbientB() const { return m_ambient.b; }

    void setAmbientR(int r);
    void setAmbientG(int g);
    void setAmbientB(int b);

    void resetScale();
    void resetRotation();
    void resetTranslation();

    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void updateRenderParams();
    static void performRaycasting(const RenderState& state, std::vector<unsigned char>& buffer, int adaptationStep);
    static std::optional<cadm::cadf> solveQuadraticMinPositive(cadm::cadf a, cadm::cadf b, cadm::cadf c);

    GLuint m_texture{};
    std::unique_ptr<ShaderProgram> m_shaderProgram;
    std::unique_ptr<quad> m_quad;
    std::vector<unsigned char> m_cpuBuffer{};

    cadm::cadf m_a{0.5}, m_b{0.2}, m_c{1};
    unsigned char m_adaptationSize{8};
    cadm::vec3 m_translation{};
    cadm::vec3 m_scale{1, 1, 1};
    cadm::vec3 m_rotation{}; /* rotation around each of the main axes */

    cadm::vec3 m_v{0, 0, -1};
    cadm::vec3 m_specularColor{1.0, 1.0, 0.0};
    cadm::vec3i m_ambient{25, 25, 25};
    cadm::cadf m_m{1};
    cadm::cadf m_sensitivity{0.001};
    cadm::cadf m_translationStep{0.1};

    QPoint m_lastMousePosition;

    Camera m_camera{cadm::vec3(0, 0, 15), cadm::vec3(0, 0, 0), cadm::vec3(0, 1, 0)};

    RenderState m_renderState;
    int m_currentAdaptationStep{1};
    bool m_zPressed{false};
};


#endif //CAD_RENDERINGWINDOW_H
