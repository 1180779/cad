//
// Created on 3/15/26.
//

#include "RenderSystem.hpp"

#include "CheckMacros.hpp"
#include "GlCommon.hpp"
#include "Scene.hpp"
#include "BezierUtils.hpp"
#include "components/BezierC0Component.hpp"
#include "components/BezierC2Component.hpp"
#include "components/InterpC2Component.hxx"
#include "components/GeometryComponent.hpp"
#include "components/TransformComponent.hpp"
#include "gui/Theme.hpp"
#include <array>
#include <cad_math/Vec2.hpp>
#include <cad_math/Vec3.hpp>
#include <cmath>
#include <concepts>

#include "components/PatchComponent.hxx"

namespace {
    /// @brief Structural interface shared by the C2 curves that render through the Bernstein
    /// patch tessellation path (BezierC2 and InterpC2)
    template <class C>
    concept bezierPatchCurve = requires(const C c) {
        { c.segmentCount() } -> std::convertible_to<int>;
        { c.getBernsteinPositions() } -> std::convertible_to<const std::vector<cadm::Vec3>&>;
        { c.getPatchVao() } -> std::convertible_to<GLuint>;
        { c.getPatchIndexCount() } -> std::convertible_to<int>;
        { c.getBernsteinPolyVao() } -> std::convertible_to<GLuint>;
        { c.getShowBernsteinCps() } -> std::convertible_to<bool>;
        { c.getShowBernsteinPolygon() } -> std::convertible_to<bool>;
        { c.getShowControlPolyline() } -> std::convertible_to<bool>;
        { c.getControlPolylineVao() } -> std::convertible_to<GLuint>;
        { c.getControlPolylineIndexCount() } -> std::convertible_to<int>;
    };
}

void RenderSystem::initialize() {
    SHADER_ATTACHING_CHECK(m_basicShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/basicShader.vert"));
    SHADER_ATTACHING_CHECK(m_basicShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/basicShader.frag"));

    SHADER_ATTACHING_CHECK(
        m_wireframeShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/wireframe/wireframeShader.vert")
    );
    SHADER_ATTACHING_CHECK(
        m_wireframeShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/wireframe/wireframeShader.frag")
    );

    SHADER_ATTACHING_CHECK(m_axesShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/axis/axisShader.vert"));
    SHADER_ATTACHING_CHECK(m_axesShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/axis/axisShader.frag"));

    SHADER_ATTACHING_CHECK(m_gridShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/grid/gridShader.vert"));
    SHADER_ATTACHING_CHECK(m_gridShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/grid/gridShader.frag"));

    SHADER_ATTACHING_CHECK(
        m_selectionRectShader->attachShaderFromFile(
            GL_VERTEX_SHADER,
            "shaders/selectionRect/selectionRectShader.vert"
        )
    );
    SHADER_ATTACHING_CHECK(
        m_selectionRectShader->attachShaderFromFile(
            GL_FRAGMENT_SHADER,
            "shaders/selectionRect/selectionRectShader.frag"
        )
    );

    SHADER_ATTACHING_CHECK(m_pointShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/point/pointShader.vert"));
    SHADER_ATTACHING_CHECK(m_pointShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/point/pointShader.frag"));

    SHADER_ATTACHING_CHECK(
        m_bezierCurveShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/bezierCurve/bezierCurveShader.vert")
    );
    SHADER_ATTACHING_CHECK(
        m_bezierCurveShader->attachShaderFromFile(
            GL_TESS_CONTROL_SHADER,
            "shaders/bezierCurve/bezierCurveShader.tesc"
        )
    );
    SHADER_ATTACHING_CHECK(
        m_bezierCurveShader->attachShaderFromFile(
            GL_TESS_EVALUATION_SHADER,
            "shaders/bezierCurve/bezierCurveShader.tese"
        )
    );
    SHADER_ATTACHING_CHECK(
        m_bezierCurveShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/bezierCurve/bezierCurveShader.frag")
    );

    SHADER_ATTACHING_CHECK(
        m_bezierSurfaceShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/bezierSurface/bezierSurfaceShader.vert")
    );
    SHADER_ATTACHING_CHECK(
        m_bezierSurfaceShader->attachShaderFromFile(
            GL_TESS_CONTROL_SHADER,
            "shaders/bezierSurface/bezierSurfaceShader.tesc"
        )
    );
    SHADER_ATTACHING_CHECK(
        m_bezierSurfaceShader->attachShaderFromFile(
            GL_TESS_EVALUATION_SHADER,
            "shaders/bezierSurface/bezierSurfaceShader.tese"
        )
    );
    SHADER_ATTACHING_CHECK(
        m_bezierSurfaceShader->attachShaderFromFile(
            GL_FRAGMENT_SHADER,
            "shaders/bezierCurve/bezierCurveShader.frag"
        )
    );

    SHADER_ATTACHING_CHECK(
        m_stereoCompositeShader->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/stereo/stereoComposite.vert")
    );
    SHADER_ATTACHING_CHECK(
        m_stereoCompositeShader->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/stereo/stereoComposite.frag")
    );

    SHADER_COMPILATION_CHECK(m_basicShader->compile());
    SHADER_COMPILATION_CHECK(m_wireframeShader->compile());
    SHADER_COMPILATION_CHECK(m_axesShader->compile());
    SHADER_COMPILATION_CHECK(m_gridShader->compile());
    SHADER_COMPILATION_CHECK(m_selectionRectShader->compile());
    SHADER_COMPILATION_CHECK(m_pointShader->compile());
    SHADER_COMPILATION_CHECK(m_bezierCurveShader->compile());
    SHADER_COMPILATION_CHECK(m_bezierSurfaceShader->compile());
    SHADER_COMPILATION_CHECK(m_stereoCompositeShader->compile());

    m_pivotAxes.m_length = 0.5f;
    m_pivotAxes.regenerateMesh();
    m_pivotAxes.syncToGpu();

    m_screenQuad = std::make_unique<Quad>();

    const auto gl = getGl();
    // UBOs
    gl->glGenBuffers(1, &m_cameraUbo);
    gl->glBindBuffer(GL_UNIFORM_BUFFER, m_cameraUbo);
    gl->glBufferData(GL_UNIFORM_BUFFER, 4 * sizeof(cadm::Mat4), nullptr, GL_DYNAMIC_DRAW);
    gl->glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_cameraUbo);

    gl->glGenBuffers(1, &m_paletteUbo);
    gl->glBindBuffer(GL_UNIFORM_BUFFER, m_paletteUbo);
    gl->glBufferData(GL_UNIFORM_BUFFER, 5 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    gl->glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_paletteUbo);
    gl->glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // selection rect
    gl->glGenVertexArrays(1, &m_selectionRectVAO);
    gl->glGenBuffers(1, &m_selectionRectVBO);
    gl->glBindVertexArray(m_selectionRectVAO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectionRectVBO);
    gl->glBufferData(GL_ARRAY_BUFFER, 4 * 2 * gc_glCadmVtSize, nullptr, GL_DYNAMIC_DRAW);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 2, gc_glCadmVtType, GL_FALSE, 2 * gc_glCadmVtSize, nullptr);
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderSystem::regenerateGeometry(const Scene &scene) {
    for (const auto &e : scene.getEntities()) {
        const auto geometry = e->getComponent<GeometryComponent>();
        if (!geometry) {
            continue;
        }
        if (auto *geo = geometry.value();
            geo->m_needsUpdate) {
            geo->regenerateMesh();
            geo->syncToGpu();
            geo->m_needsUpdate = false;
        }
    }
}

void RenderSystem::render(
    Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    const cadm::Mat4 &invVp,
    const bool sceneVisible
) const {
    const auto gl = getGl();

    uploadCameraUbo(view, projection, invVp);
    uploadPaletteUbo();

    renderInfiniteGrid(view);
    gl->glDepthFunc(GL_LEQUAL);
    renderInfiniteAxes();
    gl->glDepthFunc(GL_LESS);

    if (!sceneVisible) {
        // the active cursor stays visible
        renderActiveCursorColors(scene);
        return;
    }

    m_wireframeShader->bind();
    const QColor &lc = theme::active().line;
    SHADER_SET_UNIFORM_CHECK(
        m_wireframeShader->setUniform4(
            "u_overrideColor",
            cadm::vec4{
            lc.redF(),
            lc.greenF(),
            lc.blueF(),
            1.0f
            }
        )
    );

    scene.getPointRegistry().syncToGpu();
    regenerateGeometry(scene);

    renderLineGeometry(scene);
    GET_GL_ERRORS();
    renderTriangleGeometry(scene);
    GET_GL_ERRORS();
    renderBezierCurves(scene, view, projection);
    GET_GL_ERRORS();
    renderPatches(scene);
    GET_GL_ERRORS();
    renderControlPoints(scene, gl);
    GET_GL_ERRORS();
    renderActiveCursorColors(scene);
    GET_GL_ERRORS();
}

void RenderSystem::renderStereo(
    Scene &scene,
    const cadm::Mat4 &leftView,
    const cadm::Mat4 &leftProjection,
    const cadm::Mat4 &rightView,
    const cadm::Mat4 &rightProjection,
    const bool luminance
) {
    const auto gl = getGl();
    GLint prevFbo = 0;
    gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    ensureStereoTargets();

    const cadm::Mat4 *views[2] = {&leftView, &rightView};
    const cadm::Mat4 *projs[2] = {&leftProjection, &rightProjection};
    for (int i = 0; i < 2; ++i) {
        gl->glBindFramebuffer(GL_FRAMEBUFFER, m_stereoFbo[i]);
        gl->glViewport(0, 0, m_stereoW, m_stereoH);
        const auto &bg = theme::active().viewport;
        gl->glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const cadm::Mat4 invVp = views[i]->inversedView() * projs[i]->inversedFrustum();
        render(scene, *views[i], *projs[i], invVp);
    }

    gl->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    gl->glViewport(0, 0, m_viewportW, m_viewportH);

    gl->glDisable(GL_DEPTH_TEST);
    m_stereoCompositeShader->bind();
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, m_stereoColor[0]);
    gl->glActiveTexture(GL_TEXTURE1);
    gl->glBindTexture(GL_TEXTURE_2D, m_stereoColor[1]);
    SHADER_SET_UNIFORM_CHECK(m_stereoCompositeShader->setUniform1("uLeft", 0));
    SHADER_SET_UNIFORM_CHECK(m_stereoCompositeShader->setUniform1("uRight", 1));
    SHADER_SET_UNIFORM_CHECK(m_stereoCompositeShader->setUniform1("uLuminance", luminance ? 1 : 0));
    m_screenQuad->draw();
    ShaderProgram::release();
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glEnable(GL_DEPTH_TEST);
}

void RenderSystem::renderSelectionRect(
    const cadm::cadf x0Ndc,
    const cadm::cadf y0Ndc,
    const cadm::cadf x1Ndc,
    const cadm::cadf y1Ndc
) const {
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

    const auto gl = getGl();
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectionRectVBO);
    gl->glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_selectionRectShader->bind();
    gl->glBindVertexArray(m_selectionRectVAO);

    // fill
    SHADER_SET_UNIFORM_CHECK(m_selectionRectShader->setUniform4("u_color", s_selectionRectColor));
    gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // outline
    SHADER_SET_UNIFORM_CHECK(m_selectionRectShader->setUniform4("u_color", s_selectionRectOutlineColor));
    gl->glDrawArrays(GL_LINE_LOOP, 0, 4);

    gl->glBindVertexArray(0);
    ShaderProgram::release();
}

void RenderSystem::renderPivotMarker(
    const cadm::Vec3 &pos
) const {
    const auto gl = getGl();
    const cadm::Mat4 model = cadm::Mat4::translation(pos);

    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", model));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform1("u_highlightStrength", s_noSelectionHS));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{}));

    gl->glBindVertexArray(m_pivotAxes.m_VAO);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pivotAxes.m_EBO_Lines);
    gl->glDrawElements(
        GL_LINES,
        static_cast<GLsizei>(m_pivotAxes.m_lineIndices.size()),
        GL_UNSIGNED_INT,
        nullptr
    );
    gl->glBindVertexArray(0);
    ShaderProgram::release();
}

void RenderSystem::renderTransformAxis(
    const cadm::Vec3 &pivot,
    const cadm::Mat4 &axisModel,
    const int axesMask
) const {
    m_axesShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniformMat4("u_model", axisModel));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform3("u_axisOrigin", pivot));
    SHADER_SET_UNIFORM_CHECK(
        m_axesShader->setUniform2(
            "u_viewport",
            cadm::vec2{static_cast<cadm::cadf>(m_viewportW), static_cast<cadm::cadf>(m_viewportH)}
        )
    );
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform1("u_lineWidth", 2.0f));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform1("u_axesMask", axesMask));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform1("u_lodFade", 0));
    m_screenQuad->draw();
    ShaderProgram::release();
}

void RenderSystem::shutdown() {
    UNIQUE_PTR_RELEASE_CHECK(m_basicShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_wireframeShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_axesShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_gridShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_selectionRectShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_pointShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_bezierCurveShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_bezierSurfaceShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_stereoCompositeShader.release());

    if (m_selectionRectVAO != 0) {
        const auto gl = getGl();
        gl->glDeleteBuffers(1, &m_selectionRectVBO);
        gl->glDeleteVertexArrays(1, &m_selectionRectVAO);
    }

    if (m_cameraUbo != 0) {
        const auto gl = getGl();
        gl->glDeleteBuffers(1, &m_cameraUbo);
        gl->glDeleteBuffers(1, &m_paletteUbo);
    }

    if (m_stereoFbo[0] != 0) {
        const auto gl = getGl();
        gl->glDeleteFramebuffers(2, m_stereoFbo);
        gl->glDeleteTextures(2, m_stereoColor);
        gl->glDeleteRenderbuffers(2, m_stereoDepth);
    }
}

void RenderSystem::renderInfiniteGrid(
    const cadm::Mat4 &view
) const {
    const cadm::Vec3 cameraForward{-view.row(2).xyz()};
    m_gridShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniform1("u_gridPlanes", m_gridPlanes));
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniform3("u_viewDir", cameraForward));
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniform1("u_lodFade", m_gridLodFade ? 1 : 0));
    m_screenQuad->draw();
    ShaderProgram::release();
}

void RenderSystem::renderInfiniteAxes() const {
    m_axesShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniformMat4("u_model", cadm::Mat4::identity()));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform3("u_axisOrigin", cadm::Vec3{}));
    SHADER_SET_UNIFORM_CHECK(
        m_axesShader->setUniform2(
            "u_viewport",
            cadm::vec2{static_cast<cadm::cadf>(m_viewportW), static_cast<cadm::cadf>(m_viewportH)}
        )
    );
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform1("u_lineWidth", 2.0f));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform1("u_axesMask", m_infiniteAxesMask));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform1("u_lodFade", m_gridLodFade ? 1 : 0));
    m_screenQuad->draw();
    ShaderProgram::release();
}

void RenderSystem::renderLineGeometry(const Scene &scene) const {
    const auto gl = getGl();
    for (const auto &e : scene.getEntities()) {
        const auto geometry = e->getComponent<GeometryComponent>();
        const auto transform = e->getComponent<TransformComponent>();
        if (!geometry || !transform) {
            continue;
        }
        const auto *pGeo = geometry.value();
        if (pGeo->m_lineIndices.empty()) {
            continue;
        }

        SHADER_SET_UNIFORM_CHECK(
            m_wireframeShader->setUniform1(
                "u_highlightStrength",
                e->isSelected()
                ? s_selectionHS
                : s_noSelectionHS
            )
        );
        SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", transform.value()->getModelMatrix()));
        gl->glBindVertexArray(pGeo->m_VAO);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pGeo->m_EBO_Lines);
        gl->glDrawElements(GL_LINES, static_cast<GLsizei>(pGeo->m_lineIndices.size()), GL_UNSIGNED_INT, nullptr);
    }
}

void RenderSystem::renderActiveCursorColors(const Scene &scene) const {
    Entity *cursor = scene.getActiveCursor();
    if (!cursor) {
        return;
    }
    const auto geometry = cursor->getComponent<GeometryComponent>();
    const auto transform = cursor->getComponent<TransformComponent>();
    if (!geometry || !transform || geometry.value()->m_lineIndices.empty()) {
        return;
    }
    const auto gl = getGl();
    const auto *pGeo = geometry.value();
    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{}));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform1("u_highlightStrength", s_noSelectionHS));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", transform.value()->getModelMatrix()));
    gl->glBindVertexArray(pGeo->m_VAO);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pGeo->m_EBO_Lines);
    gl->glDepthFunc(GL_LEQUAL);
    gl->glDrawElements(GL_LINES, static_cast<GLsizei>(pGeo->m_lineIndices.size()), GL_UNSIGNED_INT, nullptr);
    gl->glDepthFunc(GL_LESS);
    gl->glBindVertexArray(0);
    ShaderProgram::release();
}

void RenderSystem::renderTriangleGeometry(const Scene &scene) const {
    const auto gl = getGl();
    gl->glDepthMask(GL_FALSE);
    for (const auto &e : scene.getEntities()) {
        const auto geometry = e->getComponent<GeometryComponent>();
        const auto transform = e->getComponent<TransformComponent>();
        if (!geometry || !transform) {
            continue;
        }
        const auto *pGeo = geometry.value();
        if (pGeo->m_triangleIndices.empty()) {
            continue;
        }

        SHADER_SET_UNIFORM_CHECK(
            m_wireframeShader->setUniform1(
                "u_highlightStrength",
                e->isSelected()
                ? s_selectionHS
                : s_noSelectionHS
            )
        );
        SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", transform.value()->getModelMatrix()));
        gl->glBindVertexArray(pGeo->m_VAO);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pGeo->m_EBO_Triangles);
        gl->glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(pGeo->m_triangleIndices.size()),
            GL_UNSIGNED_INT,
            nullptr
        );
    }
    gl->glDepthMask(GL_TRUE);

    gl->glBindVertexArray(0);
    ShaderProgram::release();
}

void RenderSystem::renderControlPoints(
    Scene &scene,
    QOpenGLFunctions_4_5_Core *const gl
) const {
    if (const auto &pointRegistry = scene.getPointRegistry();
        !pointRegistry.empty()) {
        m_pointShader->bind();
        gl->glBindVertexArray(pointRegistry.getVAO());
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pointRegistry.getEBO());
        gl->glDrawElements(
            GL_POINTS,
            static_cast<GLsizei>(pointRegistry.aliveCount()),
            GL_UNSIGNED_INT,
            nullptr
        );
        gl->glBindVertexArray(0);
        ShaderProgram::release();
    }
}

void RenderSystem::renderC0BezierCurves(
    Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    const auto gl = getGl();

    const auto renderTesselated = [&](
        const Entity *e,
        const BezierC0Component *pBezier
    ) {
        if (const int segments = pBezier->segmentCount();
            segments > 0 || pBezier->trailingEdges() > 0) {
            SHADER_SET_UNIFORM_CHECK(
                m_bezierCurveShader->setUniform1(
                    "u_highlightStrength",
                    e->isSelected()
                    ? s_selectionHS
                    : s_noSelectionHS
                )
            );

            const int trailing = pBezier->trailingEdges();
            const int totalPatches = segments + (trailing > 0
                                                     ? 1
                                                     : 0);

            const auto &cps = pBezier->getControlPoints();
            const auto &registry = scene.getPointRegistry();

            gl->glPatchParameteri(GL_PATCH_VERTICES, 4);
            gl->glBindVertexArray(pBezier->getPatchVao());

            // render each patch with its own adaptive tessellation count
            SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniform1("uLastPrimitive", 0));
            for (int p = 0; p < totalPatches; ++p) {
                const int base = p * 3;
                const cadm::Vec3 pts[4] = {
                    registry.getPosition(cps[base]),
                    registry.getPosition(cps[base + 1]),
                    registry.getPosition(cps[std::min(base + 2, static_cast<int>(cps.size()) - 1)]),
                    registry.getPosition(cps[std::min(base + 3, static_cast<int>(cps.size()) - 1)]),
                };
                const auto pixelsOpt = bezierUtils::screenExtent(pts, view, projection, m_viewportW, m_viewportH);
                if (!pixelsOpt.has_value()) {
                    continue;
                }

                const auto pixels = pixelsOpt.value();
                const int numInstances = std::max(
                    1,
                    static_cast<int>(std::ceil(static_cast<cadm::cadf>(pixels) / 64.0f))
                );
                const bool isTrailing = p == totalPatches - 1 && trailing > 0;
                SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniform1("numInstances", numInstances));
                SHADER_SET_UNIFORM_CHECK(
                    m_bezierCurveShader->setUniform1(
                        "uLastDegree",
                        isTrailing
                        ? trailing
                        : 3
                    )
                );
                gl->glDrawElementsInstanced(
                    GL_PATCHES,
                    4,
                    GL_UNSIGNED_INT,
                    reinterpret_cast<const void*>(static_cast<uintptr_t>(p) * 4 * sizeof(uint32_t)),
                    numInstances
                );
            }
        }
    };

    const auto renderControlPolygon = [&](
        const Entity *e,
        const BezierC0Component *pBezier
    ) {
        if (pBezier->getShowPolygon() && pBezier->getPolygonIndexCount() >= 2) {
            SHADER_SET_UNIFORM_CHECK(
                m_wireframeShader->setUniform1(
                    "u_highlightStrength",
                    e->isSelected()
                    ? s_selectionHS
                    : s_noSelectionHS
                )
            );
            static constexpr cadm::vec4 polygonColor{0.3f, 0.6f, 1.0f, 1.0f};
            SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", polygonColor));
            gl->glBindVertexArray(pBezier->getPolygonVao());
            gl->glDrawElements(
                GL_LINE_STRIP,
                pBezier->getPolygonIndexCount(),
                GL_UNSIGNED_INT,
                nullptr
            );
        }
    };

    const auto forEachBezier = [&](const auto &doWork) {
        for (const auto &e : scene.getEntities()) {
            if (const auto bezier = e->getComponent<BezierC0Component>()) {
                doWork(e.get(), bezier.value());
            }
        }
    };

    // control polygons
    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", cadm::Mat4::identity()));
    forEachBezier(renderControlPolygon);
    gl->glBindVertexArray(0);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{}));
    ShaderProgram::release();

    // tessellated curves
    m_bezierCurveShader->bind();
    forEachBezier(renderTesselated);
}

void RenderSystem::renderC2BezierCurves(
    const Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    const auto gl = getGl();

    const auto forEachBezier = [&](const auto &doWork) {
        for (const auto &e : scene.getEntities()) {
            if (const auto bezier = e->getComponent<BezierC2Component>()) {
                doWork(e.get(), bezier.value());
            }
            if (const auto bezier = e->getComponent<InterpC2Component>()) {
                doWork(e.get(), bezier.value());
            }
        }
    };
    const auto highlight = [&](const Entity *e) {
        return e->isSelected()
                   ? s_selectionHS
                   : s_noSelectionHS;
    };

    // tessellated curves
    m_bezierCurveShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniform1("uLastDegree", 3));
    SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniform1("uLastPrimitive", 0));
    gl->glPatchParameteri(GL_PATCH_VERTICES, 4);
    forEachBezier(
        [&](const Entity *e, const bezierPatchCurve auto *pBezier) {
            const int segments = pBezier->segmentCount();
            if (segments <= 0) {
                return;
            }
            const auto &bps = pBezier->getBernsteinPositions();
            SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniform1("u_highlightStrength", highlight(e)));
            gl->glBindVertexArray(pBezier->getPatchVao());
            for (int p = 0; p < segments; ++p) {
                const cadm::Vec3 pts[4] = {
                    bps[3 * p + 0],
                    bps[3 * p + 1],
                    bps[3 * p + 2],
                    bps[3 * p + 3],
                };
                const auto pixelsOpt = bezierUtils::screenExtent(pts, view, projection, m_viewportW, m_viewportH);
                if (!pixelsOpt.has_value()) {
                    continue;
                }

                const auto pixels = pixelsOpt.value();
                const int numInstances = std::max(
                    1,
                    static_cast<int>(std::ceil(static_cast<cadm::cadf>(pixels) / 64.0f))
                );
                SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniform1("numInstances", numInstances));
                gl->glDrawElementsInstanced(
                    GL_PATCHES,
                    4,
                    GL_UNSIGNED_INT,
                    reinterpret_cast<const void*>(static_cast<uintptr_t>(p) * 4 * sizeof(uint32_t)),
                    numInstances
                );
            }
        }
    );
    gl->glBindVertexArray(0);
    ShaderProgram::release();

    // overlays (bernstein points/polygon, de Boor polygon)
    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", cadm::Mat4::identity()));
    const auto drawWire = [&](
        const cadm::vec4 &color,
        const GLuint vao,
        const GLenum mode,
        const int count
    ) {
        SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", color));
        gl->glBindVertexArray(vao);
        gl->glDrawElements(mode, count, GL_UNSIGNED_INT, nullptr);
    };
    forEachBezier(
        [&](const Entity *e, const bezierPatchCurve auto *pBezier) {
            SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform1("u_highlightStrength", highlight(e)));
            if (pBezier->segmentCount() > 0) {
                if (pBezier->getShowBernsteinCps()) {
                    gl->glPointSize(6.0f);
                    drawWire(
                        {0.9f, 0.9f, 0.2f, 1.0f},
                        pBezier->getPatchVao(),
                        GL_POINTS,
                        pBezier->getPatchIndexCount()
                    );
                    gl->glPointSize(1.0f);
                }
                if (pBezier->getShowBernsteinPolygon()) {
                    drawWire(
                        {0.9f, 0.8f, 0.1f, 1.0f},
                        pBezier->getBernsteinPolyVao(),
                        GL_LINE_STRIP,
                        pBezier->getPatchIndexCount()
                    );
                }
            }
            if (pBezier->getShowControlPolyline() && pBezier->getControlPolylineIndexCount() >= 2) {
                drawWire(
                    {0.9f, 0.6f, 0.1f, 1.0f},
                    pBezier->getControlPolylineVao(),
                    GL_LINE_STRIP,
                    pBezier->getControlPolylineIndexCount()
                );
            }
        }
    );
    gl->glBindVertexArray(0);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{}));
    ShaderProgram::release();
}

void RenderSystem::renderBezierCurves(
    Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    renderC0BezierCurves(scene, view, projection);
    renderC2BezierCurves(scene, view, projection);
}

void RenderSystem::drawPatchSurface(const PatchComponent *patch, const cadm::cadf entityHl) const {
    // assumes m_bezierSurfaceShader is bound and glPatchParameteri(16) is set.
    // each single patch is drawn as constant-parameter iso-lines, once per direction
    // (uDir 0/1); one draw instance per iso-line
    const auto gl = getGl();
    const int patches = patch->getPatchCount();
    if (patches <= 0) {
        return;
    }
    const int lines = patch->getGridDivisions() + 1;
    SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader->setUniform1("uLines", lines));
    SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader->setUniform1("u_highlightStrength", entityHl));
    gl->glBindVertexArray(patch->getPatchVao());
    for (int dir = 0; dir < 2; ++dir) {
        SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader->setUniform1("uDir", dir));
        for (int q = 0; q < patches; ++q) {
            gl->glDrawElementsInstanced(
                GL_PATCHES,
                16,
                GL_UNSIGNED_INT,
                reinterpret_cast<const void*>(static_cast<uintptr_t>(q) * 16 * sizeof(uint32_t)),
                lines
            );
        }
    }
    gl->glBindVertexArray(0);
}

void RenderSystem::drawPatchNet(const PatchComponent *patch, const cadm::cadf hl) const {
    // assumes m_wireframeShader is bound with an identity model
    if (!patch->getShowNet() || patch->getNetIndexCount() < 2) {
        return;
    }
    const auto gl = getGl();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform1("u_highlightStrength", hl));
    SHADER_SET_UNIFORM_CHECK(
        m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{0.9f, 0.6f, 0.1f, 1.0f})
    );
    gl->glBindVertexArray(patch->getNetVao());
    gl->glDrawElements(GL_LINES, patch->getNetIndexCount(), GL_UNSIGNED_INT, nullptr);
    gl->glBindVertexArray(0);
}

void RenderSystem::renderPatches(const Scene &scene) const {
    const auto gl = getGl();

    const auto highlight = [&](const Entity *e) {
        return e->isSelected()
                   ? s_selectionHS
                   : s_noSelectionHS;
    };

    const auto forEachPatch = [&](const auto &doWork) {
        for (const auto &e : scene.getEntities()) {
            if (!e->isVisible()) {
                continue;
            }
            if (const auto patch = e->getComponent<PatchComponent>()) {
                doWork(e.get(), patch.value());
            }
        }
    };

    m_bezierSurfaceShader->bind();
    gl->glPatchParameteri(GL_PATCH_VERTICES, 16);
    forEachPatch(
        [&](const Entity *e, const PatchComponent *patch) {
            drawPatchSurface(patch, highlight(e));
        }
    );
    ShaderProgram::release();

    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", cadm::Mat4::identity()));
    forEachPatch(
        [&](const Entity *e, const PatchComponent *patch) {
            drawPatchNet(patch, highlight(e));
        }
    );
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{}));
    ShaderProgram::release();
}

void RenderSystem::renderPreviewPatch(const PatchComponent &patch, const PointRegistry &registry) const {
    const auto gl = getGl();

    m_bezierSurfaceShader->bind();
    gl->glPatchParameteri(GL_PATCH_VERTICES, 16);
    drawPatchSurface(&patch, s_selectionHS);
    ShaderProgram::release();

    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", cadm::Mat4::identity()));
    drawPatchNet(&patch, s_selectionHS);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{}));
    ShaderProgram::release();

    if (!registry.empty()) {
        m_pointShader->bind();
        gl->glBindVertexArray(registry.getVAO());
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, registry.getEBO());
        gl->glDrawElements(
            GL_POINTS,
            static_cast<GLsizei>(registry.aliveCount()),
            GL_UNSIGNED_INT,
            nullptr
        );
        gl->glBindVertexArray(0);
        ShaderProgram::release();
    }
}

void RenderSystem::uploadCameraUbo(
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    const cadm::Mat4 &invVp
) const {
    const auto gl = getGl();
    const cadm::Mat4 vp = projection * view;
    constexpr int s = sizeof(cadm::Mat4);
    gl->glBindBuffer(GL_UNIFORM_BUFFER, m_cameraUbo);
    gl->glBufferSubData(GL_UNIFORM_BUFFER, 0 * s, s, view.data);
    gl->glBufferSubData(GL_UNIFORM_BUFFER, 1 * s, s, projection.data);
    gl->glBufferSubData(GL_UNIFORM_BUFFER, 2 * s, s, vp.data);
    gl->glBufferSubData(GL_UNIFORM_BUFFER, 3 * s, s, invVp.data);
    gl->glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void RenderSystem::uploadPaletteUbo() const {
    const theme::ThemeColors &t = theme::active();
    const auto rgba = [](const QColor &c) {
        return std::array{
            c.redF(),
            c.greenF(),
            c.blueF(),
            c.alphaF()
        };
    };
    const std::array colors{
        rgba(t.line),
        rgba(t.point),
        rgba(t.curve),
        rgba(t.gridMinor),
        rgba(t.gridMajor)
    };
    const auto gl = getGl();
    gl->glBindBuffer(GL_UNIFORM_BUFFER, m_paletteUbo);
    gl->glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(colors), colors.data());
    gl->glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void RenderSystem::ensureStereoTargets() {
    if (m_stereoFbo[0] != 0 && m_stereoW == m_viewportW && m_stereoH == m_viewportH) {
        return;
    }

    const auto gl = getGl();
    if (m_stereoFbo[0] != 0) {
        gl->glDeleteFramebuffers(2, m_stereoFbo);
        gl->glDeleteTextures(2, m_stereoColor);
        gl->glDeleteRenderbuffers(2, m_stereoDepth);
    }

    m_stereoW = m_viewportW;
    m_stereoH = m_viewportH;

    gl->glGenFramebuffers(2, m_stereoFbo);
    gl->glGenTextures(2, m_stereoColor);
    gl->glGenRenderbuffers(2, m_stereoDepth);
    for (int i = 0; i < 2; ++i) {
        gl->glBindFramebuffer(GL_FRAMEBUFFER, m_stereoFbo[i]);

        gl->glBindTexture(GL_TEXTURE_2D, m_stereoColor[i]);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_stereoW, m_stereoH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_stereoColor[i], 0);

        gl->glBindRenderbuffer(GL_RENDERBUFFER, m_stereoDepth[i]);
        gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_stereoW, m_stereoH);
        gl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_stereoDepth[i]);
    }
    gl->glBindTexture(GL_TEXTURE_2D, 0);
    gl->glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
