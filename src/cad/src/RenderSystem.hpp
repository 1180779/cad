//
// Created on 3/15/26.
//

#ifndef CAD_RENDERSYSTEM_H
#define CAD_RENDERSYSTEM_H

#include "ShaderProgram.hpp"
#include "Quad.hpp"
#include "components/GeometryComponent.hpp"
#include <memory>
#include <cad_math/vec3.hpp>
#include <cad_math/mat4.hpp>

class Scene;

class RenderSystem
{
public:
    void initialize();
    static void regenerateGeometry(const Scene &scene);

    void render(Scene &scene, const cadm::mat4 &view, const cadm::mat4 &projection, const cadm::mat4 &invVp) const;

    void renderSelectionRect(
        cadm::cadf x0Ndc,
        cadm::cadf y0Ndc,
        cadm::cadf x1Ndc,
        cadm::cadf y1Ndc) const;
    void renderPivotMarker(
        const cadm::vec3 &pos,
        const cadm::mat4 &view,
        const cadm::mat4 &projection) const;
    void renderTransformAxis(
        const cadm::vec3 &pivot,
        const cadm::mat4 &axisModel,
        int axesMask,
        const cadm::mat4 &view,
        const cadm::mat4 &projection,
        const cadm::mat4 &invVp
    ) const;

    void shutdown();

    // bitmask: bit 0 = XY (z=0), bit 1 = XZ (y=0), bit 2 = YZ (x=0)
    void setGridPlanes(const int planes) { m_gridPlanes = planes; }
    [[nodiscard]] int getGridPlanes() const { return m_gridPlanes; }

    void setViewport(const int w, const int h)
    {
        m_viewportW = w;
        m_viewportH = h;
    }

private:
    void renderInfiniteGrid(const cadm::mat4 &view, const cadm::mat4 &projection, const cadm::mat4 &invVp) const;

    void renderInfiniteAxes(
        const cadm::mat4 &view,
        const cadm::mat4 &projection,
        const cadm::mat4 &invVp
    ) const;

    void renderLineGeometry(const Scene &scene, QOpenGLFunctions_4_5_Core *gl) const;
    void renderTriangleGeometry(const Scene &scene, QOpenGLFunctions_4_5_Core *gl) const;
    void renderControlPoints(
        Scene &scene,
        const cadm::mat4 &view,
        const cadm::mat4 &projection,
        QOpenGLFunctions_4_5_Core *gl) const;
    void renderC0BezierCurves(
        Scene &scene,
        const cadm::mat4 &view,
        const cadm::mat4 &projection,
        const cadm::mat4 &vp
    ) const;

    void renderC2BezierCurves(
        const Scene &scene,
        const cadm::mat4 &view,
        const cadm::mat4 &projection,
        const cadm::mat4 &vp
    ) const;

    void renderBezierCurves(
        Scene &scene,
        const cadm::mat4 &view,
        const cadm::mat4 &projection) const;

    AxesGeometry m_pivotAxes;

    std::unique_ptr<ShaderProgram> m_basicShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_wireframeShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_axesShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_gridShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_selectionRectShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_pointShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_bezierCurveShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<Quad> m_screenQuad;

    int m_gridPlanes{1};
    int m_viewportW{1};
    int m_viewportH{1};

    // 2D selection rectangle
    uint32_t m_selectionRectVAO = 0;
    uint32_t m_selectionRectVBO = 0;
    static constexpr cadm::vec4 s_selectionRectColor{0.39, 0.63, 1.0, 0.16};
    static constexpr cadm::vec4 s_selectionRectOutlineColor{1.0, 1.0, 1.0, 0.86};

public:
    static constexpr cadm::cadf s_selectionHS{0.7f}; // highlight strength
    static constexpr cadm::cadf s_noSelectionHS{0.0f};
};

#endif //CAD_RENDERSYSTEM_H
