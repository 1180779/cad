//
// Created on 3/15/26.
//

#include "RenderSystem.hpp"

#include "CheckMacros.hpp"
#include "GlCommon.hpp"
#include "Scene.hpp"
#include "components/GeometryComponent.hpp"
#include "components/TransformComponent.hpp"


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

    SHADER_ATTACHING_CHECK(
        m_selectionRectShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/selectionRectShader.vert"));
    SHADER_ATTACHING_CHECK(
        m_selectionRectShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/selectionRectShader.frag"));

    SHADER_ATTACHING_CHECK(m_pointShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/pointShader.vert"));
    SHADER_ATTACHING_CHECK(m_pointShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/pointShader.frag"));


    SHADER_COMPILATION_CHECK(m_basicShader->compile());
    SHADER_COMPILATION_CHECK(m_wireframeShader->compile());
    SHADER_COMPILATION_CHECK(m_axesShader->compile());
    SHADER_COMPILATION_CHECK(m_gridShader->compile());
    SHADER_COMPILATION_CHECK(m_selectionRectShader->compile());
    SHADER_COMPILATION_CHECK(m_pointShader->compile());

    m_pivotAxes.m_length = 0.5f;
    m_pivotAxes.regenerateMesh();
    m_pivotAxes.syncToGpu();

    m_screenQuad = std::make_unique<Quad>();

    // Selection rect
    const auto gl = GL();
    gl->glGenVertexArrays(1, &m_selectionRectVAO);
    gl->glGenBuffers(1, &m_selectionRectVBO);
    gl->glBindVertexArray(m_selectionRectVAO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectionRectVBO);
    gl->glBufferData(GL_ARRAY_BUFFER, 4 * 2 * GL_CADM_VT_SIZE, nullptr, GL_DYNAMIC_DRAW);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 2, GL_CADM_VT_TYPE, GL_FALSE, 2 * GL_CADM_VT_SIZE, nullptr);
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderSystem::renderInfiniteGrid(
    const cadm::mat4 &view,
    const cadm::mat4 &projection,
    const cadm::mat4 &invVP) const
{
    const auto VP = projection * view;
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

void RenderSystem::renderLineGeometry(const Scene &scene, QOpenGLFunctions_4_5_Core *const gl) const
{
    for (const auto &e : scene.getEntities())
    {
        const auto geometry = e->getComponent<GeometryComponent>();
        const auto transform = e->getComponent<TransformComponent>();
        if (!geometry || !transform) continue;
        const auto *pGeo = geometry.value();
        if (pGeo->m_lineIndices.empty()) continue;

        SHADER_SET_UNIFORM_CHECK(
            m_wireframeShader->setUniform1("u_highlightStrength", e->isSelected() ? s_selectionHS :
                s_noSelectionHS));
        SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", transform.value()->getModelMatrix()));
        gl->glBindVertexArray(pGeo->m_VAO);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pGeo->m_EBO_Lines);
        gl->glDrawElements(GL_LINES, static_cast<GLsizei>(pGeo->m_lineIndices.size()), GL_UNSIGNED_INT, nullptr);
    }
}

void RenderSystem::renderTriangleGeometry(const Scene &scene, QOpenGLFunctions_4_5_Core *const gl) const
{
    gl->glDepthMask(GL_FALSE);
    for (const auto &e : scene.getEntities())
    {
        const auto geometry = e->getComponent<GeometryComponent>();
        const auto transform = e->getComponent<TransformComponent>();
        if (!geometry || !transform) continue;
        const auto *pGeo = geometry.value();
        if (pGeo->m_triangleIndices.empty()) continue;

        SHADER_SET_UNIFORM_CHECK(
            m_wireframeShader->setUniform1("u_highlightStrength", e->isSelected() ? s_selectionHS :
                s_noSelectionHS));
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
}

void RenderSystem::renderControlPoints(
    Scene &scene,
    const cadm::mat4 &view,
    const cadm::mat4 &projection,
    QOpenGLFunctions_4_5_Core *const gl) const
{
    auto &pointRegistry = scene.getPointRegistry();
    pointRegistry.syncToGpu();
    if (!pointRegistry.empty())
    {
        m_pointShader->bind();
        SHADER_SET_UNIFORM_CHECK(m_pointShader->setUniformMat4("view", view));
        SHADER_SET_UNIFORM_CHECK(m_pointShader->setUniformMat4("projection", projection));
        gl->glBindVertexArray(pointRegistry.getVAO());
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pointRegistry.getEBO());
        gl->glDrawElements(
            GL_POINTS,
            static_cast<GLsizei>(pointRegistry.aliveCount()),
            GL_UNSIGNED_INT,
            nullptr);
        gl->glBindVertexArray(0);
        m_pointShader->release();
    }
}

void RenderSystem::render(
    Scene &scene,
    const cadm::mat4 &view,
    const cadm::mat4 &projection,
    const cadm::mat4 &invVP) const
{
    const auto gl = GL();

    renderInfiniteGrid(view, projection, invVP);

    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("view", view));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("projection", projection));

    regenerateGeometry(scene);

    renderLineGeometry(scene, gl);
    GET_GL_ERRORS();
    renderTriangleGeometry(scene, gl);
    GET_GL_ERRORS();
    renderControlPoints(scene, view, projection, gl);

    GET_GL_ERRORS();
}

void RenderSystem::renderSelectionRect(
    const cadm::cadf x0Ndc,
    const cadm::cadf y0Ndc,
    const cadm::cadf x1Ndc,
    const cadm::cadf y1Ndc) const
{
    const cadm::cadf verts[8] = {
        x0Ndc,
        y0Ndc,
        x1Ndc,
        y0Ndc,
        x1Ndc,
        y1Ndc,
        x0Ndc,
        y1Ndc,
    };

    const auto gl = GL();
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectionRectVBO);
    gl->glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_selectionRectShader->bind();
    gl->glBindVertexArray(m_selectionRectVAO);

    // Fill
    SHADER_SET_UNIFORM_CHECK(m_selectionRectShader->setUniform4("u_color", s_selectionRectColor));
    gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Outline
    SHADER_SET_UNIFORM_CHECK(m_selectionRectShader->setUniform4("u_color", s_selectionRectOutlineColor));
    gl->glDrawArrays(GL_LINE_LOOP, 0, 4);

    gl->glBindVertexArray(0);
    m_selectionRectShader->release();
}

void RenderSystem::renderPivotMarker(
    const cadm::vec3 &pos,
    const cadm::mat4 &view,
    const cadm::mat4 &projection) const
{
    const auto gl = GL();
    const cadm::mat4 model = cadm::mat4::translation(pos);

    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("view", view));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("projection", projection));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", model));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform1("u_highlightStrength", s_noSelectionHS));

    gl->glBindVertexArray(m_pivotAxes.m_VAO);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pivotAxes.m_EBO_Lines);
    gl->glDrawElements(
        GL_LINES,
        static_cast<GLsizei>(m_pivotAxes.m_lineIndices.size()),
        GL_UNSIGNED_INT,
        nullptr);
    gl->glBindVertexArray(0);
    m_wireframeShader->release();
}

void RenderSystem::shutdown()
{
    UNIQUE_PTR_RELEASE_CHECK(m_basicShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_wireframeShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_axesShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_gridShader.release());

    if (m_selectionRectVAO != 0)
    {
        const auto gl = GL();
        gl->glDeleteBuffers(1, &m_selectionRectVBO);
        gl->glDeleteVertexArrays(1, &m_selectionRectVAO);
    }
}
