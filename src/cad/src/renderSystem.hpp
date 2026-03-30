//
// Created on 3/15/26.
//

#ifndef CAD_RENDERSYSTEM_H
#define CAD_RENDERSYSTEM_H

#include "shaderProgram.hpp"
#include "quad.hpp"
#include <memory>

class Scene;

class RenderSystem
{
public:
    void initialize();
    void renderInfiniteGrid(const cadm::mat4 &view, const cadm::mat4 &projection) const;
    void regenerateGeometry(const Scene &scene);
    void render(const Scene &scene, const cadm::mat4 &view, const cadm::mat4 &projection);
    void renderSelectionRect(
        cadm::cadf x0Ndc,
        cadm::cadf y0Ndc,
        cadm::cadf x1Ndc,
        cadm::cadf y1Ndc) const;
    void shutdown();

    // bitmask: bit 0 = XY (z=0), bit 1 = XZ (y=0), bit 2 = YZ (x=0)
    void setGridPlanes(const int planes) { m_gridPlanes = planes; }
    [[nodiscard]] int getGridPlanes() const { return m_gridPlanes; }

private:
    int m_gridPlanes{1};

    std::unique_ptr<shaderProgram> m_basicShader = std::make_unique<shaderProgram>();
    std::unique_ptr<shaderProgram> m_wireframeShader = std::make_unique<shaderProgram>();
    std::unique_ptr<shaderProgram> m_axesShader = std::make_unique<shaderProgram>();
    std::unique_ptr<shaderProgram> m_gridShader = std::make_unique<shaderProgram>();
    std::unique_ptr<shaderProgram> m_selectionRectShader = std::make_unique<shaderProgram>();
    std::unique_ptr<quad> m_screenQuad;

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
