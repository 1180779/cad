//
// Created on 3/15/26.
//

#include "renderSystem.hpp"

#include "checkMacros.hpp"
#include "scene.hpp"
#include "components/geometry.hpp"
#include "components/transform.hpp"


void RenderSystem::initialize()
{
    SHADER_ATTACHING_CHECK(m_basicShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/basicShader.vert"));
    SHADER_ATTACHING_CHECK(m_basicShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/basicShader.frag"));

    SHADER_ATTACHING_CHECK(m_wireframeShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/wireframeShader.vert"));
    SHADER_ATTACHING_CHECK(m_wireframeShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/wireframeShader.frag"));

    SHADER_ATTACHING_CHECK(m_axesShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/axesShader.vert"));
    SHADER_ATTACHING_CHECK(m_axesShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/axesShader.frag"));

    SHADER_ATTACHING_CHECK(m_gridShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/gridShader.vert"));
    SHADER_ATTACHING_CHECK(m_gridShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/gridShader.frag"));

    SHADER_COMPILATION_CHECK(m_basicShader->compile());
    SHADER_COMPILATION_CHECK(m_wireframeShader->compile());
    SHADER_COMPILATION_CHECK(m_axesShader->compile());
    SHADER_COMPILATION_CHECK(m_gridShader->compile());

    m_screenQuad = std::make_unique<quad>();
}

void RenderSystem::renderInfiniteGrid(const cadm::mat4 &view, const cadm::mat4 &projection) const
{
    const auto VP = projection * view;
    const auto invVP = VP.inversed();
    m_gridShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniformMat4("VP", VP));
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniformMat4("invVP", invVP));
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniform1("u_gridPlanes", m_gridPlanes));
    m_screenQuad->draw();
    m_gridShader->release();
}

void RenderSystem::regenerateGeometry(const Scene &scene)
{
    for (const auto &e : scene.getEntities())
    {
        const auto geometry = e->getComponent<GeometryComponent>();
        if (!geometry) continue;
        if (auto *geo = geometry.value(); geo->m_needsUpdate)
        {
            geo->regenerateMesh();
            geo->syncToGpu();
            geo->m_needsUpdate = false;
        }
    }
}

void RenderSystem::render(const Scene &scene, const cadm::mat4 &view, const cadm::mat4 &projection)
{
    const auto gl = GL();

    renderInfiniteGrid(view, projection);

    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("view", view));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("projection", projection));

    regenerateGeometry(scene);

    // Pass 1: line geometry
    for (const auto &e : scene.getEntities())
    {
        const auto geometry = e->getComponent<GeometryComponent>();
        const auto transform = e->getComponent<TransformComponent>();
        if (!geometry || !transform) continue;
        const auto *pGeo = geometry.value();
        if (pGeo->m_lineIndices.empty()) continue;

        SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", transform.value()->getModelMatrix()));
        gl->glBindVertexArray(pGeo->m_VAO);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pGeo->m_EBO_Lines);
        gl->glDrawElements(GL_LINES, static_cast<GLsizei>(pGeo->m_lineIndices.size()), GL_UNSIGNED_INT, nullptr);
    }

    // Pass 2: triangle geometry
    gl->glDepthMask(GL_FALSE);
    for (const auto &e : scene.getEntities())
    {
        const auto geometry = e->getComponent<GeometryComponent>();
        const auto transform = e->getComponent<TransformComponent>();
        if (!geometry || !transform) continue;
        const auto *pGeo = geometry.value();
        if (pGeo->m_triangleIndices.empty()) continue;

        SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", transform.value()->getModelMatrix()));
        gl->glBindVertexArray(pGeo->m_VAO);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pGeo->m_EBO_Triangles);
        gl->glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(pGeo->m_triangleIndices.size()),
            GL_UNSIGNED_INT,
            nullptr);
    }
    gl->glDepthMask(GL_TRUE);

    gl->glBindVertexArray(0);
    m_wireframeShader->release();
    GET_GL_ERRORS();
}

void RenderSystem::shutdown()
{
    UNIQUE_PTR_RELEASE_CHECK(m_basicShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_wireframeShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_axesShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_gridShader.release());
}
