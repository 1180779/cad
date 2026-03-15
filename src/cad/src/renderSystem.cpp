//
// Created on 3/15/26.
//

#include "renderSystem.h"

#include "checkMacros.hpp"
#include "scene.h"
#include "components/geometry.h"
#include "components/transform.h"
#include <QDebug> // Include QDebug for logging


void RenderSystem::initialize() const
{
    SHADER_ATTACHING_CHECK(m_basicShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/basicShader.vert"));
    SHADER_ATTACHING_CHECK(m_basicShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/basicShader.frag"));

    SHADER_ATTACHING_CHECK(m_wireframeShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/wireframeShader.vert"));
    SHADER_ATTACHING_CHECK(m_wireframeShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/wireframeShader.frag"));

    SHADER_ATTACHING_CHECK(m_axesShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/axesShader.vert"));
    SHADER_ATTACHING_CHECK(m_axesShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/axesShader.frag"));

    SHADER_COMPILATION_CHECK(m_basicShader->compile());
    SHADER_COMPILATION_CHECK(m_wireframeShader->compile());
    SHADER_COMPILATION_CHECK(m_axesShader->compile());
}

void RenderSystem::render(const Scene &scene, const camera &camera)
{
    // TODO: refactor to use first entity with camera component?
    const auto gl = GL();
    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("view", camera.getViewMatrix()));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("projection", camera.getProjectionMatrix()));
    for (const auto &e : scene.getEntities())
    {
        const auto geometry = e->getComponent<GeometryComponent>();
        const auto transform = e->getComponent<TransformComponent>();

        if (!geometry || !transform) continue;

        SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", transform.value()->getModelMatrix()));

        gl->glBindVertexArray(geometry.value()->m_VAO);
        gl->glDrawElements(
            geometry.value()->m_drawMode,
            static_cast<GLsizei>(geometry.value()->m_indices.size()),
            GL_UNSIGNED_INT,
            nullptr);
    }
    m_wireframeShader->release();

    GET_GL_ERRORS();
}

void RenderSystem::shutdown()
{
    UNIQUE_PTR_RELEASE_CHECK(m_basicShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_wireframeShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_axesShader.release());
}
