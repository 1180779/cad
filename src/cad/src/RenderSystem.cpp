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
#include "components/GeometryComponent.hpp"
#include "components/TransformComponent.hpp"
#include <cad_math/Vec2.hpp>
#include <cad_math/Vec3.hpp>
#include <cmath>

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

    SHADER_COMPILATION_CHECK(m_basicShader->compile());
    SHADER_COMPILATION_CHECK(m_wireframeShader->compile());
    SHADER_COMPILATION_CHECK(m_axesShader->compile());
    SHADER_COMPILATION_CHECK(m_gridShader->compile());
    SHADER_COMPILATION_CHECK(m_selectionRectShader->compile());
    SHADER_COMPILATION_CHECK(m_pointShader->compile());
    SHADER_COMPILATION_CHECK(m_bezierCurveShader->compile());

    m_pivotAxes.m_length = 0.5f;
    m_pivotAxes.regenerateMesh();
    m_pivotAxes.syncToGpu();

    m_screenQuad = std::make_unique<Quad>();

    // selection rect

    const auto gl = getGl();
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

void RenderSystem::renderInfiniteGrid(
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    const cadm::Mat4 &invVp
) const {
    const auto vp = projection * view;
    const cadm::Vec3 cameraForward{-view.row(2).xyz()};
    m_gridShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniformMat4("VP", vp));
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniformMat4("invVP", invVp));
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniform1("u_gridPlanes", m_gridPlanes));
    SHADER_SET_UNIFORM_CHECK(m_gridShader->setUniform3("u_viewDir", cameraForward));
    m_screenQuad->draw();
    m_gridShader->release();
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

void RenderSystem::renderLineGeometry(const Scene &scene, QOpenGLFunctions_4_5_Core *const gl) const {
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

void RenderSystem::renderTriangleGeometry(const Scene &scene, QOpenGLFunctions_4_5_Core *const gl) const {
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
    m_wireframeShader->release();
}

void RenderSystem::renderControlPoints(
    Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    QOpenGLFunctions_4_5_Core *const gl
) const {
    const auto &pointRegistry = scene.getPointRegistry();
    if (!pointRegistry.empty()) {
        m_pointShader->bind();
        SHADER_SET_UNIFORM_CHECK(m_pointShader->setUniformMat4("view", view));
        SHADER_SET_UNIFORM_CHECK(m_pointShader->setUniformMat4("projection", projection));
        gl->glBindVertexArray(pointRegistry.getVAO());
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pointRegistry.getEBO());
        gl->glDrawElements(
            GL_POINTS,
            static_cast<GLsizei>(pointRegistry.aliveCount()),
            GL_UNSIGNED_INT,
            nullptr
        );
        gl->glBindVertexArray(0);
        m_pointShader->release();
    }
}

void RenderSystem::renderC0BezierCurves(
    Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    const cadm::Mat4 &vp
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
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("view", view));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("projection", projection));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("model", cadm::Mat4::identity()));
    forEachBezier(renderControlPolygon);
    gl->glBindVertexArray(0);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{}));
    m_wireframeShader->release();

    // tessellated curves
    m_bezierCurveShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniformMat4("MVP", vp));
    forEachBezier(renderTesselated);
}

void RenderSystem::renderC2BezierCurves(
    const Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    const cadm::Mat4 &vp
) const {
    const auto gl = getGl();

    const auto forEachBezier = [&](const auto &doWork) {
        for (const auto &e : scene.getEntities()) {
            if (const auto bezier = e->getComponent<BezierC2Component>()) {
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
    SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniformMat4("MVP", vp));
    SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniform1("uLastDegree", 3));
    SHADER_SET_UNIFORM_CHECK(m_bezierCurveShader->setUniform1("uLastPrimitive", 0));
    gl->glPatchParameteri(GL_PATCH_VERTICES, 4);
    forEachBezier(
        [&](const Entity *e, const BezierC2Component *pBezier) {
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
    m_bezierCurveShader->release();

    // overlays (bernstein points/polygon, de Boor polygon)
    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("view", view));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("projection", projection));
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
        [&](const Entity *e, const BezierC2Component *pBezier) {
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
            if (pBezier->getShowDeBoorPolygon() && pBezier->getDeBoorIndexCount() >= 2) {
                drawWire(
                    {0.9f, 0.6f, 0.1f, 1.0f},
                    pBezier->getDeBoorVao(),
                    GL_LINE_STRIP,
                    pBezier->getDeBoorIndexCount()
                );
            }
        }
    );
    gl->glBindVertexArray(0);
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniform4("u_overrideColor", cadm::vec4{}));
    m_wireframeShader->release();
}

void RenderSystem::renderBezierCurves(
    Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    const cadm::Mat4 vp = projection * view;
    renderC0BezierCurves(scene, view, projection, vp);
    renderC2BezierCurves(scene, view, projection, vp);
}

void RenderSystem::renderInfiniteAxes(
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    const cadm::Mat4 &invVp
) const {
    const cadm::Mat4 vp = projection * view;
    m_axesShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniformMat4("VP", vp));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniformMat4("invVP", invVp));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniformMat4("u_model", cadm::Mat4::identity()));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform3("u_axisOrigin", cadm::Vec3{}));
    SHADER_SET_UNIFORM_CHECK(
        m_axesShader->setUniform2(
            "u_viewport",
            cadm::vec2{static_cast<cadm::cadf>(m_viewportW), static_cast<cadm::cadf>(m_viewportH)}
        )
    );
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform1("u_lineWidth", 2.0f));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniform1("u_axesMask", 7));
    m_screenQuad->draw();
    m_axesShader->release();
}

void RenderSystem::renderTransformAxis(
    const cadm::Vec3 &pivot,
    const cadm::Mat4 &axisModel,
    const int axesMask,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    const cadm::Mat4 &invVp
) const {
    const cadm::Mat4 vp = projection * view;
    m_axesShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniformMat4("VP", vp));
    SHADER_SET_UNIFORM_CHECK(m_axesShader->setUniformMat4("invVP", invVp));
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
    m_screenQuad->draw();
    m_axesShader->release();
}

void RenderSystem::render(
    Scene &scene,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection,
    const cadm::Mat4 &invVp
) const {
    const auto gl = getGl();

    renderInfiniteGrid(view, projection, invVp);

    gl->glDepthFunc(GL_LEQUAL);
    renderInfiniteAxes(view, projection, invVp);
    gl->glDepthFunc(GL_LESS);

    m_wireframeShader->bind();
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("view", view));
    SHADER_SET_UNIFORM_CHECK(m_wireframeShader->setUniformMat4("projection", projection));

    scene.getPointRegistry().syncToGpu();
    regenerateGeometry(scene);

    renderLineGeometry(scene, gl);
    GET_GL_ERRORS();
    renderTriangleGeometry(scene, gl);
    GET_GL_ERRORS();
    renderBezierCurves(scene, view, projection);
    GET_GL_ERRORS();
    renderControlPoints(scene, view, projection, gl);

    GET_GL_ERRORS();
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
    m_selectionRectShader->release();
}

void RenderSystem::renderPivotMarker(
    const cadm::Vec3 &pos,
    const cadm::Mat4 &view,
    const cadm::Mat4 &projection
) const {
    const auto gl = getGl();
    const cadm::Mat4 model = cadm::Mat4::translation(pos);

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
        nullptr
    );
    gl->glBindVertexArray(0);
    m_wireframeShader->release();
}

void RenderSystem::shutdown() {
    UNIQUE_PTR_RELEASE_CHECK(m_basicShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_wireframeShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_axesShader.release());
    UNIQUE_PTR_RELEASE_CHECK(m_gridShader.release());

    if (m_selectionRectVAO != 0) {
        const auto gl = getGl();
        gl->glDeleteBuffers(1, &m_selectionRectVBO);
        gl->glDeleteVertexArrays(1, &m_selectionRectVAO);
    }
}
