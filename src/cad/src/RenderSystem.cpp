//
// Created on 3/15/26.
//

#include "RenderSystem.hpp"

#include "cad_graphics/Checks.hxx"
#include "GlCommon.hpp"
#include "Scene.hpp"
#include "utils/BezierUtils.hpp"
#include "components/geometry/BezierC0Component.hpp"
#include "components/geometry/BezierC2Component.hpp"
#include "components/geometry/InterpC2Component.hxx"
#include "components/GeometryComponent.hpp"
#include "components/TransformComponent.hpp"
#include "gui/Theme.hpp"
#include <algorithm>
#include <array>
#include <cad_math/Vec2.hxx>
#include <cad_math/Vec3.hxx>
#include <cmath>
#include <vector>
#include <concepts>
#include <filesystem>
#include <initializer_list>
#include <utility>
#include <QCoreApplication>

#include "components/geometry/GregoryComponent.hxx"
#include "components/geometry/PatchComponent.hxx"

namespace {
    /// @brief Structural interface shared by the C2 curves that render through the Bernstein
    /// patch tessellation path (BezierC2 and InterpC2)
    template <typename C>
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
    const auto gl = getGl();
    const auto shaderDirectory = std::filesystem::path(QCoreApplication::applicationDirPath().toStdWString()) / "shaders";
    const auto loadProgram = [gl](std::initializer_list<cadg::ShaderFile> files) {
        auto program = cadg::ShaderProgram::fromFiles(*gl, std::span{files.begin(), files.size()});
        if (!program) {
            qFatal("Failed to load CAD shader program: %s", program.error().c_str());
        }
        return std::move(*program);
    };

    m_basicShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "basicShader.vert"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "basicShader.frag"},
    });
    m_wireframeShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "wireframe/wireframeShader.vert"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "wireframe/wireframeShader.frag"},
    });
    m_axesShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "axis/axisShader.vert"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "axis/axisShader.frag"},
    });
    m_gridShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "grid/gridShader.vert"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "grid/gridShader.frag"},
    });
    m_selectionRectShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "selectionRect/selectionRectShader.vert"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "selectionRect/selectionRectShader.frag"},
    });
    m_pointShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "point/pointShader.vert"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "point/pointShader.frag"},
    });
    m_bezierCurveShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "bezierCurve/bezierCurveShader.vert"},
        {cadg::ShaderStage::TessControl, shaderDirectory / "bezierCurve/bezierCurveShader.tesc"},
        {cadg::ShaderStage::TessEvaluation, shaderDirectory / "bezierCurve/bezierCurveShader.tese"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "bezierCurve/bezierCurveShader.frag"},
    });
    m_bezierSurfaceShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "bezierSurface/bezierSurfaceShader.vert"},
        {cadg::ShaderStage::TessControl, shaderDirectory / "bezierSurface/bezierSurfaceShader.tesc"},
        {cadg::ShaderStage::TessEvaluation, shaderDirectory / "bezierSurface/bezierSurfaceShader.tese"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "bezierSurface/bezierSurfaceShader.frag"},
    });
    m_gregorySurfaceShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "bezierSurface/bezierSurfaceShader.vert"},
        {cadg::ShaderStage::TessControl, shaderDirectory / "gregorySurface/gregorySurfaceShader.tesc"},
        {cadg::ShaderStage::TessEvaluation, shaderDirectory / "gregorySurface/gregorySurfaceShader.tese"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "bezierCurve/bezierCurveShader.frag"},
    });
    m_stereoCompositeShader = loadProgram({
        {cadg::ShaderStage::Vertex, shaderDirectory / "stereo/stereoComposite.vert"},
        {cadg::ShaderStage::Fragment, shaderDirectory / "stereo/stereoComposite.frag"},
    });

    m_pivotAxes.m_length = 0.5f;
    m_pivotAxes.updateIfNecessary();
    m_screenQuad = std::make_unique<Quad>();

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
        geometry.value()->updateIfNecessary();
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

    m_wireframeShader.bind(*gl);
    const QColor &lc = theme::active().line;
    SHADER_SET_UNIFORM_CHECK(
        m_wireframeShader.uniform4f(
            *gl,
            "u_overrideColor",
            cadm::Vec4{
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
    renderPatches(scene, view, projection);
    GET_GL_ERRORS();
    renderGregory(scene, view, projection);
    GET_GL_ERRORS();
    renderControlPoints(scene, gl);
    GET_GL_ERRORS();
    renderActiveCursorColors(scene);
    GET_GL_ERRORS();
}

void RenderSystem::renderStereo(
    Scene &scene,
    const std::span<cadm::Mat4, 2> views,
    const std::span<cadm::Mat4, 2> projs,
    const bool luminance,
    const bool sceneVisible,
    const std::function<void(const cadm::Mat4 &view, const cadm::Mat4 &projection)> &perEye
) {
    const auto gl = getGl();
    GLint prevFbo = 0;
    gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    ensureStereoTargets();

    for (int i = 0; i < 2; ++i) {
        gl->glBindFramebuffer(GL_FRAMEBUFFER, m_stereoFbo[i]);
        gl->glViewport(0, 0, m_stereoW, m_stereoH);
        const auto &bg = theme::active().viewport;
        gl->glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const cadm::Mat4 invVp = views[i].inversedView() * projs[i].inversedFrustum();
        render(scene, views[i], projs[i], invVp, sceneVisible);
        if (perEye) {
            perEye(views[i], projs[i]);
        }
    }

    gl->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    gl->glViewport(0, 0, m_viewportW, m_viewportH);

    gl->glDisable(GL_DEPTH_TEST);
    m_stereoCompositeShader.bind(*gl);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, m_stereoColor[0]);
    gl->glActiveTexture(GL_TEXTURE1);
    gl->glBindTexture(GL_TEXTURE_2D, m_stereoColor[1]);
    SHADER_SET_UNIFORM_CHECK(m_stereoCompositeShader.uniform1i(*gl, "uLeft", 0));
    SHADER_SET_UNIFORM_CHECK(m_stereoCompositeShader.uniform1i(*gl, "uRight", 1));
    SHADER_SET_UNIFORM_CHECK(m_stereoCompositeShader.uniform1i(*gl, "uLuminance", luminance ? 1 : 0));
    m_screenQuad->draw();
    cadg::ShaderProgram::unbind(*gl);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glEnable(GL_DEPTH_TEST);
}

void RenderSystem::renderBoxSelectionRect(
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

    m_selectionRectShader.bind(*gl);
    gl->glBindVertexArray(m_selectionRectVAO);

    // fill
    SHADER_SET_UNIFORM_CHECK(m_selectionRectShader.uniform4f(*gl, "u_color", s_selectionRectColor));
    gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // outline
    SHADER_SET_UNIFORM_CHECK(m_selectionRectShader.uniform4f(*gl, "u_color", s_selectionRectOutlineColor));
    gl->glDrawArrays(GL_LINE_LOOP, 0, 4);

    gl->glBindVertexArray(0);
    cadg::ShaderProgram::unbind(*gl);
}

void RenderSystem::renderPivotMarker(
    const cadm::Vec3 &pos
) const {
    const auto gl = getGl();
    const cadm::Mat4 model = cadm::Mat4::translation(pos);

    m_wireframeShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", model));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform1f(*gl, "u_highlightStrength", s_noSelectionHS));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{}));

    gl->glBindVertexArray(m_pivotAxes.m_VAO);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pivotAxes.m_EBO_Lines);
    gl->glDrawElements(
        GL_LINES,
        static_cast<GLsizei>(m_pivotAxes.m_lineIndices.size()),
        GL_UNSIGNED_INT,
        nullptr
    );
    gl->glBindVertexArray(0);
    cadg::ShaderProgram::unbind(*gl);
}

void RenderSystem::renderTransformAxis(
    const cadm::Vec3 &pivot,
    const cadm::Mat4 &axisModel,
    const int axesMask
) const {
    const auto gl = getGl();
    m_axesShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniformMatrix4f(*gl, "u_model", axisModel));
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniform3f(*gl, "u_axisOrigin", pivot));
    SHADER_SET_UNIFORM_CHECK(
        m_axesShader.uniform2f(
            *gl,
            "u_viewport",
            cadm::Vec2{static_cast<cadm::cadf>(m_viewportW), static_cast<cadm::cadf>(m_viewportH)}
        )
    );
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniform1f(*gl, "u_lineWidth", 2.0f));
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniform1i(*gl, "u_axesMask", axesMask));
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniform1i(*gl, "u_lodFade", 0));
    m_screenQuad->draw();
    cadg::ShaderProgram::unbind(*gl);
}

void RenderSystem::shutdown() {
    const auto gl = getGl();
    for (auto* program : {
        &m_basicShader, &m_wireframeShader, &m_axesShader, &m_gridShader, &m_selectionRectShader,
        &m_pointShader, &m_bezierCurveShader, &m_bezierSurfaceShader, &m_gregorySurfaceShader,
        &m_stereoCompositeShader,
    }) {
        program->destroy(*gl);
    }

    if (m_selectionRectVAO != 0) {
        gl->glDeleteBuffers(1, &m_selectionRectVBO);
        gl->glDeleteVertexArrays(1, &m_selectionRectVAO);
    }

    if (m_cameraUbo != 0) {
        gl->glDeleteBuffers(1, &m_cameraUbo);
        gl->glDeleteBuffers(1, &m_paletteUbo);
    }

    if (m_stereoFbo[0] != 0) {
        gl->glDeleteFramebuffers(2, m_stereoFbo);
        gl->glDeleteTextures(2, m_stereoColor);
        gl->glDeleteRenderbuffers(2, m_stereoDepth);
    }
}

void RenderSystem::renderInfiniteGrid(
    const cadm::Mat4 &view
) const {
    const auto gl = getGl();
    const cadm::Vec3 cameraForward{-view.row(2).xyz()};
    m_gridShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_gridShader.uniform1i(*gl, "u_gridPlanes", m_gridPlanes));
    SHADER_SET_UNIFORM_CHECK(m_gridShader.uniform3f(*gl, "u_viewDir", cameraForward));
    SHADER_SET_UNIFORM_CHECK(m_gridShader.uniform1i(*gl, "u_lodFade", m_gridLodFade ? 1 : 0));
    m_screenQuad->draw();
    cadg::ShaderProgram::unbind(*gl);
}

void RenderSystem::renderInfiniteAxes() const {
    const auto gl = getGl();
    m_axesShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniformMatrix4f(*gl, "u_model", cadm::Mat4::identity()));
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniform3f(*gl, "u_axisOrigin", cadm::Vec3{}));
    SHADER_SET_UNIFORM_CHECK(
        m_axesShader.uniform2f(
            *gl,
            "u_viewport",
            cadm::Vec2{static_cast<cadm::cadf>(m_viewportW), static_cast<cadm::cadf>(m_viewportH)}
        )
    );
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniform1f(*gl, "u_lineWidth", 2.0f));
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniform1i(*gl, "u_axesMask", m_infiniteAxesMask));
    SHADER_SET_UNIFORM_CHECK(m_axesShader.uniform1i(*gl, "u_lodFade", m_gridLodFade ? 1 : 0));
    m_screenQuad->draw();
    cadg::ShaderProgram::unbind(*gl);
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
            m_wireframeShader.uniform1f(
                *gl,
                "u_highlightStrength",
                e->isSelected()
                ? s_selectionHS
                : s_noSelectionHS
            )
        );
        SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", transform.value()->getModelMatrix()));
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
    m_wireframeShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{}));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform1f(*gl, "u_highlightStrength", s_noSelectionHS));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", transform.value()->getModelMatrix()));
    gl->glBindVertexArray(pGeo->m_VAO);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pGeo->m_EBO_Lines);
    gl->glDepthFunc(GL_LEQUAL);
    gl->glDrawElements(GL_LINES, static_cast<GLsizei>(pGeo->m_lineIndices.size()), GL_UNSIGNED_INT, nullptr);
    gl->glDepthFunc(GL_LESS);
    gl->glBindVertexArray(0);
    cadg::ShaderProgram::unbind(*gl);
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
            m_wireframeShader.uniform1f(
                *gl,
                "u_highlightStrength",
                e->isSelected()
                ? s_selectionHS
                : s_noSelectionHS
            )
        );
        SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", transform.value()->getModelMatrix()));
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
    cadg::ShaderProgram::unbind(*gl);
}

void RenderSystem::renderControlPoints(
    Scene &scene,
    QOpenGLFunctions_4_5_Core *const gl
) const {
    if (const auto &pointRegistry = scene.getPointRegistry();
        !pointRegistry.empty()) {
        m_pointShader.bind(*gl);
        gl->glBindVertexArray(pointRegistry.getVAO());
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pointRegistry.getEBO());
        gl->glDrawElements(
            GL_POINTS,
            static_cast<GLsizei>(pointRegistry.aliveCount()),
            GL_UNSIGNED_INT,
            nullptr
        );
        gl->glBindVertexArray(0);
        cadg::ShaderProgram::unbind(*gl);
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
                m_bezierCurveShader.uniform1f(
                    *gl,
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
            SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader.uniform1i(*gl, "uLastPrimitive", 0));
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
                SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader.uniform1i(*gl, "numInstances", numInstances));
                SHADER_SET_UNIFORM_CHECK(
                    m_bezierCurveShader.uniform1i(
                        *gl,
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
                m_wireframeShader.uniform1f(
                    *gl,
                    "u_highlightStrength",
                    e->isSelected()
                    ? s_selectionHS
                    : s_noSelectionHS
                )
            );
            static constexpr cadm::Vec4 polygonColor{0.3f, 0.6f, 1.0f, 1.0f};
            SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", polygonColor));
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
    m_wireframeShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", cadm::Mat4::identity()));
    forEachBezier(renderControlPolygon);
    gl->glBindVertexArray(0);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{}));
    cadg::ShaderProgram::unbind(*gl);

    // tessellated curves
    m_bezierCurveShader.bind(*gl);
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
    m_bezierCurveShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader.uniform1i(*gl, "uLastDegree", 3));
    SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader.uniform1i(*gl, "uLastPrimitive", 0));
    gl->glPatchParameteri(GL_PATCH_VERTICES, 4);
    forEachBezier(
        [&](const Entity *e, const bezierPatchCurve auto *pBezier) {
            const int segments = pBezier->segmentCount();
            if (segments <= 0) {
                return;
            }
            const auto &bps = pBezier->getBernsteinPositions();
            SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader.uniform1f(*gl, "u_highlightStrength", highlight(e)));
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
                SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader.uniform1i(*gl, "numInstances", numInstances));
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
    cadg::ShaderProgram::unbind(*gl);

    // overlays (bernstein points/polygon, de Boor polygon)
    m_wireframeShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", cadm::Mat4::identity()));
    const auto drawWire = [&](
        const cadm::Vec4 &color,
        const GLuint vao,
        const GLenum mode,
        const int count
    ) {
        SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", color));
        gl->glBindVertexArray(vao);
        gl->glDrawElements(mode, count, GL_UNSIGNED_INT, nullptr);
    };
    forEachBezier(
        [&](const Entity *e, const bezierPatchCurve auto *pBezier) {
            SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform1f(*gl, "u_highlightStrength", highlight(e)));
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
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{}));
    cadg::ShaderProgram::unbind(*gl);
}

void RenderSystem::renderBezierCurves(
    Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    renderC0BezierCurves(scene, view, projection);
    renderC2BezierCurves(scene, view, projection);
}

void RenderSystem::drawPatchSurface(
    const PatchComponent *patch,
    const cadm::cadf entityHl,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    // assumes m_bezierSurfaceShader is bound and glPatchParameteri(16) is set
    const auto gl = getGl();
    const int patches = patch->getPatchCount();
    if (patches <= 0) {
        return;
    }
    SHADER_SET_UNIFORM_CHECK(
        m_bezierSurfaceShader.uniform2f(
            *gl,
            "uViewport",
            static_cast<float>(m_viewportW),
            static_cast<float>(m_viewportH)
        )
    );
    // per-quad slice counts from the screen extent of its 16 control points
    // (0 = fully off-screen, skip)
    const auto &indices = patch->getPatchIndices();
    std::vector<int> subs(patches);
    for (int q = 0; q < patches; ++q) {
        cadm::Vec3 pts[16];
        for (int k = 0; k < 16; ++k) {
            pts[k] = patch->patchVertexPos(indices[static_cast<size_t>(q) * 16 + k]);
        }
        const auto extent = bezierUtils::screenExtent(pts, view, projection, m_viewportW, m_viewportH);
        subs[q] = extent.has_value()
                      ? std::max(1,
                          static_cast<int>(std::ceil(
                              static_cast<cadm::cadf>(*extent) / static_cast<cadm::cadf>(m_performanceConfig.
                                  uSubBase))))
                      : 0;
    }

    patch->syncTrimToGpu();
    const bool trimmed = patch->getTrim().enabled && patch->getTrimTexture() != 0;
    SHADER_SET_UNIFORM_CHECK(
        m_bezierSurfaceShader.uniform1i(
            *gl,
            "uTrimEnabled",
            trimmed
            ? 1
            : 0
        )
    );
    SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader.uniform1i(*gl, "uCountX", patch->getPatchCountX()));
    SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader.uniform1i(*gl, "uCountY", patch->getPatchCountY()));
    SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader.uniform1f(*gl, "uMinTessLevel", m_performanceConfig.minTessLevel));
    SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader.uniform1f(*gl, "uMaxTessLevel", m_performanceConfig.maxTessLevel));
    if (trimmed) {
        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, patch->getTrimTexture());
        SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader.uniform1i(*gl, "uTrimMask", 0));
    }

    gl->glBindVertexArray(patch->getPatchVao());
    for (int dir = 0; dir < 2; ++dir) {
        const int lines = (dir == 0
                               ? patch->getGridDivisionsV()
                               : patch->getGridDivisionsU()) + 1;
        SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader.uniform1i(*gl, "uLines", lines));
        SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader.uniform1i(*gl, "uDir", dir));
        for (int q = 0; q < patches; ++q) {
            if (subs[q] == 0) {
                continue;
            }
            SHADER_SET_UNIFORM_CHECK(
                m_bezierSurfaceShader.uniform1f(
                    *gl,
                    "u_highlightStrength",
                    patch->isPatchSelected(q)
                    ? s_singlePatchSelectionHS
                    : entityHl
                )
            );
            SHADER_SET_UNIFORM_CHECK(m_bezierSurfaceShader.uniform1i(*gl, "uSub", subs[q]));
            // quads are emitted row-major over (patchCountY, patchCountX), see
            // PatchComponent::patchCoords
            SHADER_SET_UNIFORM_CHECK(
                m_bezierSurfaceShader.uniform1i(*gl, "uQuadX", q % std::max(1, patch->getPatchCountX()))
            );
            SHADER_SET_UNIFORM_CHECK(
                m_bezierSurfaceShader.uniform1i(*gl, "uQuadY", q / std::max(1, patch->getPatchCountX()))
            );
            gl->glDrawElementsInstanced(
                GL_PATCHES,
                16,
                GL_UNSIGNED_INT,
                reinterpret_cast<const void*>(static_cast<uintptr_t>(q) * 16 * sizeof(uint32_t)),
                lines * subs[q]
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
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform1f(*gl, "u_highlightStrength", hl));
    SHADER_SET_UNIFORM_CHECK(
        m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{0.9f, 0.6f, 0.1f, 1.0f})
    );
    gl->glBindVertexArray(patch->getNetVao());
    gl->glDrawElements(GL_LINES, patch->getNetIndexCount(), GL_UNSIGNED_INT, nullptr);
    gl->glBindVertexArray(0);
}

void RenderSystem::renderPatches(
    const Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
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

    m_bezierSurfaceShader.bind(*gl);
    gl->glPatchParameteri(GL_PATCH_VERTICES, 16);
    forEachPatch(
        [&](const Entity *e, const PatchComponent *patch) {
            drawPatchSurface(patch, highlight(e), view, projection);
        }
    );
    cadg::ShaderProgram::unbind(*gl);

    m_wireframeShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", cadm::Mat4::identity()));
    forEachPatch(
        [&](const Entity *e, const PatchComponent *patch) {
            drawPatchNet(patch, highlight(e));
        }
    );
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{}));
    cadg::ShaderProgram::unbind(*gl);
}

void RenderSystem::drawGregorySurface(
    const GregoryComponent *gregory,
    const cadm::cadf entityHl,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    // assumes m_gregorySurfaceShader is bound and glPatchParameteri(20) is set
    const auto gl = getGl();
    const int nets = gregory->netCount();
    constexpr int netPts = gregory::Net::pointCount;
    if (nets <= 0) {
        return;
    }
    SHADER_SET_UNIFORM_CHECK(
        m_gregorySurfaceShader.uniform2f(
            *gl,
            "uViewport",
            static_cast<float>(m_viewportW),
            static_cast<float>(m_viewportH)
        )
    );
    std::vector<int> subs(nets);
    for (int q = 0; q < nets; ++q) {
        cadm::Vec3 pts[netPts];
        for (int k = 0; k < netPts; ++k) {
            pts[k] = gregory->patchVertexPos(q * netPts + k);
        }
        const auto extent = bezierUtils::screenExtent(pts, view, projection, m_viewportW, m_viewportH);
        subs[q] = extent.has_value()
                      ? std::max(1,
                          static_cast<int>(std::ceil(
                              static_cast<cadm::cadf>(*extent) / static_cast<cadm::cadf>(m_performanceConfig.uSubBase))))
                      : 0;
    }

    SHADER_SET_UNIFORM_CHECK(m_gregorySurfaceShader.uniform1f(*gl, "u_highlightStrength", entityHl));
    SHADER_SET_UNIFORM_CHECK(m_gregorySurfaceShader.uniform1f(*gl, "uMinTessLevel", m_performanceConfig.minTessLevel));
    SHADER_SET_UNIFORM_CHECK(m_gregorySurfaceShader.uniform1f(*gl, "uMaxTessLevel", m_performanceConfig.maxTessLevel));
    gl->glBindVertexArray(gregory->getPatchVao());
    for (int dir = 0; dir < 2; ++dir) {
        SHADER_SET_UNIFORM_CHECK(m_gregorySurfaceShader.uniform1i(*gl, "uDir", dir));
        for (int net = 0; net < nets; ++net) {
            if (subs[net] == 0) {
                continue;
            }
            const int lines = (dir == 0
                                   ? gregory->getGridDivisionsV(net)
                                   : gregory->getGridDivisionsU(net)) + 1;
            SHADER_SET_UNIFORM_CHECK(m_gregorySurfaceShader.uniform1i(*gl, "uLines", lines));
            SHADER_SET_UNIFORM_CHECK(m_gregorySurfaceShader.uniform1i(*gl, "uSub", subs[net]));
            gl->glDrawElementsInstanced(
                GL_PATCHES,
                netPts,
                GL_UNSIGNED_INT,
                reinterpret_cast<const void*>(static_cast<uintptr_t>(net) * netPts * sizeof(uint32_t)),
                lines * subs[net]
            );
        }
    }
    gl->glBindVertexArray(0);
}

void RenderSystem::renderGregory(
    const Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    const auto gl = getGl();

    m_gregorySurfaceShader.bind(*gl);
    gl->glPatchParameteri(GL_PATCH_VERTICES, 20);
    const auto forEachGregory = [&](const auto &doWork) {
        for (const auto &e : scene.getEntities()) {
            if (!e->isVisible()) {
                continue;
            }
            if (const auto gregory = e->getComponent<GregoryComponent>()) {
                doWork(
                    gregory.value(),
                    e->isSelected()
                        ? s_selectionHS
                        : s_noSelectionHS
                );
            }
        }
    };

    forEachGregory(
        [&](const GregoryComponent *gregory, const cadm::cadf hl) {
            drawGregorySurface(gregory, hl, view, projection);
        }
    );
    cadg::ShaderProgram::unbind(*gl);

    m_wireframeShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", cadm::Mat4::identity()));
    SHADER_SET_UNIFORM_CHECK(
        m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{0.2f, 0.75f, 0.9f, 1.0f})
    );
    forEachGregory(
        [&](const GregoryComponent *gregory, const cadm::cadf hl) {
            if (!gregory->getShowVectors() || gregory->getVectorsIndexCount() < 2) {
                return;
            }
            SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform1f(*gl, "u_highlightStrength", hl));
            gl->glBindVertexArray(gregory->getVectorsVao());
            gl->glDrawElements(GL_LINES, gregory->getVectorsIndexCount(), GL_UNSIGNED_INT, nullptr);
            gl->glBindVertexArray(0);
        }
    );
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{}));
    cadg::ShaderProgram::unbind(*gl);
}

void RenderSystem::renderPreviewPatch(
    const PatchComponent &patch,
    const PointRegistry &registry,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    const auto gl = getGl();

    m_bezierSurfaceShader.bind(*gl);
    gl->glPatchParameteri(GL_PATCH_VERTICES, 16);
    drawPatchSurface(&patch, s_selectionHS, view, projection);
    cadg::ShaderProgram::unbind(*gl);

    m_wireframeShader.bind(*gl);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniformMatrix4f(*gl, "model", cadm::Mat4::identity()));
    drawPatchNet(&patch, s_selectionHS);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader.uniform4f(*gl, "u_overrideColor", cadm::Vec4{}));
    cadg::ShaderProgram::unbind(*gl);

    if (!registry.empty()) {
        m_pointShader.bind(*gl);
        gl->glBindVertexArray(registry.getVAO());
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, registry.getEBO());
        gl->glDrawElements(
            GL_POINTS,
            static_cast<GLsizei>(registry.aliveCount()),
            GL_UNSIGNED_INT,
            nullptr
        );
        gl->glBindVertexArray(0);
        cadg::ShaderProgram::unbind(*gl);
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
