//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_RENDERINGWINDOW_H
#define CAD_RENDERINGWINDOW_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <memory>

#include <cad_math/common.h>
#include "GL.h"
#include "ShaderProgram.h"
#include "quad.h"
#include "cad_math/vec3.h"
#include "cad_math/vec3i.h"

class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget* parent = nullptr);

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

private:
    void fillCpuBufferTest();
    void fillCpuBuffer();
    static std::optional<cadm::cadf> solveQuadratic(cadm::cadf a, cadm::cadf b, cadm::cadf c);

    GLuint m_texture{};
    std::unique_ptr<ShaderProgram> m_shaderProgram;
    std::unique_ptr<quad> m_quad;
    std::vector<unsigned char> m_cpuBuffer{};

    cadm::cadf m_a{1}, m_b{1}, m_c{1};
    unsigned char m_adaptationSize{8};
    cadm::vec3 m_translation{};
    cadm::vec3 m_scale{1, 1, 1};
    cadm::vec3 m_rotation{}; /* rotation around each of the main axes */

    cadm::vec3 m_v { 0, 0, 1 };
    cadm::vec3 m_specularColor { 1.0, 1.0, 0.0 };
    cadm::vec3i m_ambient { 25, 25, 25 };
    cadm::cadf m_m { 1 };
};


#endif //CAD_RENDERINGWINDOW_H
