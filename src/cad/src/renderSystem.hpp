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
    std::unique_ptr<quad> m_screenQuad;

public:
    static constexpr cadm::cadf s_selectionHS{0.7f}; // highlight strength
    static constexpr cadm::cadf s_noSelectionHS{0.0f};
};

#endif //CAD_RENDERSYSTEM_H
