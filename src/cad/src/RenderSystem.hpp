//
// Created on 3/15/26.
//

#ifndef CAD_RENDERSYSTEM_H
#define CAD_RENDERSYSTEM_H

#include "ShaderProgram.hpp"
#include "Quad.hpp"
#include "components/GeometryComponent.hpp"
#include <memory>
#include <cad_math/Vec3.hpp>
#include <cad_math/Mat4.hpp>

class Scene;

class RenderSystem {
public:
    void initialize();

    static void regenerateGeometry(const Scene &scene);

    void render(
        Scene &scene,
        const cadm::Mat4 &view,
        const cadm::Mat4 &projection,
        const cadm::Mat4 &invVp
    ) const;

    void renderStereo(
        Scene &scene,
        const cadm::Mat4 &leftView,
        const cadm::Mat4 &leftProjection,
        const cadm::Mat4 &rightView,
        const cadm::Mat4 &rightProjection
    );

    void renderSelectionRect(
        cadm::cadf x0Ndc,
        cadm::cadf y0Ndc,
        cadm::cadf x1Ndc,
        cadm::cadf y1Ndc
    ) const;

    void renderPivotMarker(
        const cadm::Vec3 &pos
    ) const;

    void renderTransformAxis(
        const cadm::Vec3 &pivot,
        const cadm::Mat4 &axisModel,
        int axesMask
    ) const;

    void shutdown();

    // bitmask: bit 0 = XY (z=0), bit 1 = XZ (y=0), bit 2 = YZ (x=0)
    void setGridPlanes(const int planes) {
        m_gridPlanes = planes;
    }

    [[nodiscard]] int getGridPlanes() const {
        return m_gridPlanes;
    }

    // bitmask: bit 0 = X, bit 1 = Y, bit 2 = Z
    void setInfiniteAxesMask(const int mask) {
        m_infiniteAxesMask = mask;
    }

    [[nodiscard]] int getInfiniteAxesMask() const {
        return m_infiniteAxesMask;
    }

    /// @brief Toggle the distance LOD fade applied to both grid and axes
    void setGridLodFade(const bool enabled) {
        m_gridLodFade = enabled;
    }

    void setViewport(const int w, const int h) {
        m_viewportW = w;
        m_viewportH = h;
    }

private:
    void renderInfiniteGrid(const cadm::Mat4 &view) const;

    void renderInfiniteAxes(
    ) const;

    void renderLineGeometry(const Scene &scene) const;

    /// @brief Redraw the active cursor with its baked RGB axis colors
    /// @note It is necessary to redraw the cursor after the rendering passes
    /// since theme color overrides the color in the vertices by default
    void renderActiveCursorColors(const Scene &scene) const;

    void renderTriangleGeometry(const Scene &scene) const;

    void renderControlPoints(
        Scene &scene,
        QOpenGLFunctions_4_5_Core *gl
    ) const;

    void renderC0BezierCurves(
        Scene &scene,
        const cadm::Mat4 &view,
        const cadm::Mat4 &projection
    ) const;

    void renderC2BezierCurves(
        const Scene &scene,
        const cadm::Mat4 &view,
        const cadm::Mat4 &projection
    ) const;

    void renderBezierCurves(
        Scene &scene,
        const cadm::Mat4 &view,
        const cadm::Mat4 &projection
    ) const;

    /// @brief Upload shared view/projection/VP/invVP into the Camera UBO
    void uploadCameraUbo(const cadm::Mat4 &view, const cadm::Mat4 &projection, const cadm::Mat4 &invVp) const;

    /// @brief Upload the active theme's geometry colors into the Palette UBO
    void uploadPaletteUbo() const;

    AxesGeometry m_pivotAxes;

    std::unique_ptr<ShaderProgram> m_basicShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_wireframeShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_axesShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_gridShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_selectionRectShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_pointShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_bezierCurveShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<ShaderProgram> m_stereoCompositeShader = std::make_unique<ShaderProgram>();
    std::unique_ptr<Quad> m_screenQuad;

    /// @brief UBO with view/projection/VP/invVP
    /// @note binding 0
    uint32_t m_cameraUbo{};

    /// @brief UBO with theme geometry colors
    /// @note binding 1
    uint32_t m_paletteUbo{};

    /// @brief Lazily (re)create the per-eye offscreen color+depth targets when the viewport size changes
    void ensureStereoTargets();

    // per-eye offscreen targets (index 0 = left, 1 = right)

    uint32_t m_stereoFbo[2]{};
    uint32_t m_stereoColor[2]{};
    uint32_t m_stereoDepth[2]{};
    int m_stereoW{0};
    int m_stereoH{0};

    int m_gridPlanes{0};
    int m_infiniteAxesMask{0};
    bool m_gridLodFade{true};
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
