//
// Created on 3/15/26.
//

#ifndef CAD_RENDERSYSTEM_H
#define CAD_RENDERSYSTEM_H

#include "shaderProgram.h"
#include <memory>

class Scene;

class RenderSystem
{
public:
    void initialize() const;
    void render(const Scene &scene, const cadm::mat4 &view, const cadm::mat4 &projection);
    void shutdown();

private:
    std::unique_ptr<shaderProgram> m_basicShader = std::make_unique<shaderProgram>();
    std::unique_ptr<shaderProgram> m_wireframeShader = std::make_unique<shaderProgram>();;
    std::unique_ptr<shaderProgram> m_axesShader = std::make_unique<shaderProgram>();;
};

#endif //CAD_RENDERSYSTEM_H