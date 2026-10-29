/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#include <cnc_ui/CncRenderSystem.hxx>

#include "cad_graphics/Checks.hxx"

#include <cad_graphics/PrimitiveMeshes.hxx>
#include <cad_graphics/ShaderProgramGlBoundBorrow.hxx>
#include <cad_graphics/VertexLayout.hxx>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <span>
#include <utility>

#include <QOpenGLContext>
#include <QOpenGLVersionFunctionsFactory>

namespace cnc::ui {
namespace {
struct SideVertex {
    cadm::Vec2 uv;
    float vertical;
    cadm::Vec3 normal;
};

struct LineVertex {
    cadm::Vec3 position;
    cadm::Vec3 color;
};

constexpr std::array gc_sideAttributes{
    cadg::VertexAttribute{
        .location = 0,
        .components = 2,
        .offset = offsetof(SideVertex, uv)
    },
    cadg::VertexAttribute{
        .location = 1,
        .components = 1,
        .offset = offsetof(SideVertex, vertical)
    },
    cadg::VertexAttribute{
        .location = 2,
        .components = 3,
        .offset = offsetof(SideVertex, normal)
    },
};

constexpr std::array gc_lineAttributes{
    cadg::VertexAttribute{
        .location = 0,
        .components = 3,
        .offset = offsetof(LineVertex, position)
    },
    cadg::VertexAttribute{
        .location = 1,
        .components = 3,
        .offset = offsetof(LineVertex, color)
    },
};

void uploadMesh(
    cadg::Gl& gl,
    cadg::GpuMesh& target,
    const cadg::MeshData& mesh
    ) {
    target.destroy(gl);
    auto uploaded = cadg::GpuMesh::upload(gl, mesh);
    assert(uploaded.has_value() && "Failed to upload a CNC preview mesh");
    if (uploaded.has_value()) {
        target = std::move(*uploaded);
    }
}

void uploadMesh(
    cadg::Gl& gl,
    cadg::GpuMesh& target,
    const cadg::MeshView& mesh
    ) {
    target.destroy(gl);
    auto uploaded = cadg::GpuMesh::upload(gl, mesh);
    assert(uploaded.has_value() && "Failed to upload a CNC preview mesh");
    if (uploaded.has_value()) {
        target = std::move(*uploaded);
    }
}

void uploadLineMesh(
    cadg::Gl& gl,
    cadg::GpuMesh& mesh,
    const std::vector<LineVertex>& vertices
    ) {
    if (vertices.empty()) {
        mesh.destroy(gl);
        return;
    }

    std::vector<std::uint32_t> indices(vertices.size());
    std::uint32_t index = 0;
    for (auto& value : indices) {
        value = index++;
    }

    uploadMesh(
        gl,
        mesh,
        cadg::makeMeshView(
            "cnc.lines",
            std::span(vertices),
            std::span<const std::uint32_t>(indices),
            {
                .stride = sizeof(LineVertex),
                .attributes = gc_lineAttributes
            })
        );
}

LineVertex lineVertex(const cadm::Vec3& xyz, const cadm::Vec3& rgb) {
    return {
        .position = xyz,
        .color = rgb,
    };
}

void appendLine(
    std::vector<LineVertex>& vertices,
    const cadm::Vec3& from,
    const cadm::Vec3& to,
    const cadm::Vec3& color
    ) {
    vertices.push_back(lineVertex(from, color));
    vertices.push_back(lineVertex(to, color));
}

cadm::Vec3 cncToScene(const cadm::Vec3& point) {
    return {point.x, point.z, -point.y};
}

}

CncRenderSystem::~CncRenderSystem() {
    assert(!m_initialized && "CncRenderSystem destroyed without shutdown()");
}

bool CncRenderSystem::initialize(
    const std::string& shaderDirectory,
    std::string* error
    ) {
    const auto fail = [error](std::string message) {
        if (error != nullptr) {
            *error = std::move(message);
        }
        return false;
    };

    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context == nullptr) {
        return fail("CncRenderSystem::initialize requires a current OpenGL context.");
    }
    m_gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(context);
    if (m_gl == nullptr || !m_gl->initializeOpenGLFunctions()) {
        return fail("OpenGL 4.5 core functions are not available.");
    }

    if (m_initialized) {
        shutdown();
    }
    if (!loadPrograms(shaderDirectory, error)) {
        shutdown();
        return false;
    }

    m_initialized = true;
    createTopVao();
    createToolMeshes();
    if (m_materialDirty) {
        rebuildSideMesh();
        m_materialDirty = false;
    }
    if (m_pathDirty) {
        rebuildPathMesh();
        m_pathDirty = false;
    }
    return true;
}

void CncRenderSystem::shutdown() {
    if (m_gl == nullptr) {
        m_initialized = false;
        return;
    }
    for (auto* mesh : {&m_sideMesh, &m_pathMesh, &m_sphereMesh, &m_cylinderMesh}) {
        mesh->destroy(*m_gl);
    }
    if (m_topVao != 0) {
        m_gl->glDeleteVertexArrays(1, &m_topVao);
        m_topVao = 0;
    }

    for (auto* program : {&m_stockProgram, &m_sideProgram, &m_lineProgram, &m_toolProgram}) {
        program->destroy(m_gl);
    }
    m_initialized = false;
    m_gl = nullptr;
}

bool CncRenderSystem::loadPrograms(
    const std::string& shaderDirectory,
    std::string* error
    ) {
    const auto fail = [error](std::string message) {
        if (error != nullptr) {
            *error = std::move(message);
        }
        return false;
    };

    const std::filesystem::path directory(shaderDirectory);
    auto programFromFiles = cadg::ShaderProgram::fromFiles(m_gl, directory / "stock.vert", directory / "stock.frag");
    if (!programFromFiles.has_value()) {
        return fail("Stock shader: " + programFromFiles.error());
    }
    m_stockProgram = std::move(programFromFiles.value());

    programFromFiles = cadg::ShaderProgram::fromFiles(m_gl, directory / "side.vert", directory / "stock.frag");
    if (!programFromFiles.has_value()) {
        return fail("Side shader: " + programFromFiles.error());
    }
    m_sideProgram = std::move(programFromFiles.value());

    programFromFiles = cadg::ShaderProgram::fromFiles(m_gl, directory / "line.vert", directory / "line.frag");
    if (!programFromFiles.has_value()) {
        return fail("Line shader: " + programFromFiles.error());
    }
    m_lineProgram = std::move(programFromFiles.value());

    programFromFiles = cadg::ShaderProgram::fromFiles(m_gl, directory / "tool.vert", directory / "stock.frag");
    if (!programFromFiles.has_value()) {
        return fail("Tool shader: " + programFromFiles.error());
    }
    m_toolProgram = std::move(programFromFiles.value());

    return true;
}

void CncRenderSystem::setMaterial(const MaterialConfig& material) {
    m_material = material.normalized();
    m_materialDirty = true;
    if (m_initialized) {
        rebuildSideMesh();
        m_materialDirty = false;
    }
}

void CncRenderSystem::setPath(std::shared_ptr<const ToolPath> path) {
    if (m_path == path) {
        return;
    }
    m_path = std::move(path);
    m_pathDirty = true;
    if (m_initialized) {
        rebuildPathMesh();
        m_pathDirty = false;
    }
}

void CncRenderSystem::createTopVao() {
    m_gl->glGenVertexArrays(1, &m_topVao);
}

void CncRenderSystem::rebuildSideMesh() {
    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();

    std::vector<SideVertex> vertices;
    std::vector<std::uint32_t> indices;

    const auto addEdge = [&](
        const int count,
        const bool alongY,
        const float fixed,
        const cadm::Vec3& normal
        ) {
        const auto base = static_cast<std::uint32_t>(vertices.size());
        const auto addVertex = [&](const float u, const float v, const float vertical) {
            vertices.push_back({
                .uv = {u, v},
                .vertical = vertical,
                .normal = normal,
            });
        };
        for (int i = 0; i < count; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(count - 1);
            const float u = alongY ? fixed : t;
            const float v = alongY ? t : fixed;
            addVertex(u, v, 0.0f);
            addVertex(u, v, 1.0f);
        }
        for (int i = 0; i < count - 1; ++i) {
            const std::uint32_t b0 = base + static_cast<std::uint32_t>(i * 2);
            const std::uint32_t t0 = b0 + 1;
            const std::uint32_t b1 = b0 + 2;
            const std::uint32_t t1 = b0 + 3;
            indices.insert(indices.end(), {b0, b1, t0, t0, b1, t1});
        }
    };

    const auto addBottomCap = [&]() {
        const auto bottomBase = static_cast<std::uint32_t>(vertices.size());
        const auto addBottomVertex = [&](const float u, const float v) {
            vertices.push_back({
                .uv = {u, v},
                .vertical = 0.0f,
                .normal = {0.0f, 0.0f, -1.0f},
            });
        };
        addBottomVertex(0.0f, 0.0f);
        addBottomVertex(1.0f, 0.0f);
        addBottomVertex(1.0f, 1.0f);
        addBottomVertex(0.0f, 1.0f);
        indices.insert(indices.end(), {
            bottomBase, bottomBase + 2, bottomBase + 1,
            bottomBase, bottomBase + 3, bottomBase + 2,
        });
    };

    addEdge(ny, true, 0.0f, {-1, 0, 0});
    addEdge(ny, true, 1.0f, {1, 0, 0});
    addEdge(nx, false, 0.0f, {0, -1, 0});
    addEdge(nx, false, 1.0f, {0, 1, 0});
    addBottomCap();

    uploadMesh(
        *m_gl,
        m_sideMesh,
        cadg::makeMeshView(
            "cnc.stock.side",
            std::span<const SideVertex>(vertices),
            std::span<const std::uint32_t>(indices),
            {.stride = sizeof(SideVertex), .attributes = gc_sideAttributes}
            )
        );
}

void CncRenderSystem::rebuildPathMesh() {
    if (!m_path) {
        m_pathMesh.destroy(*m_gl);
        return;
    }

    constexpr cadm::Vec3 cuttingColor{0.05f, 0.78f, 0.98f};
    constexpr cadm::Vec3 rapidColor{0.98f, 0.55f, 0.10f};
    std::vector<LineVertex> vertices;
    vertices.reserve(m_path->size() * 2);
    for (const Move& move : *m_path) {
        if (move.from == move.to) {
            continue;
        }
        appendLine(
            vertices,
            cncToScene(move.from),
            cncToScene(move.to),
            move.type == MoveType::G00 ? rapidColor : cuttingColor
            );
    }
    uploadLineMesh(*m_gl, m_pathMesh, vertices);
}

void CncRenderSystem::createToolMeshes() {
    uploadMesh(*m_gl, m_sphereMesh, cadg::makeSphere(1.0f, 24, 12));
    uploadMesh(*m_gl, m_cylinderMesh, cadg::makeCylinder(1.0f, 1.0f, 24, 1));
}

void CncRenderSystem::drawStock(
    const gpu::GpuHeightMap& heightMap,
    const cadm::Mat4& viewProjection,
    const cadm::Vec3& camPos,
    const bool flatShading
    ) const {
    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();
    const cadm::Vec3 minimum{
        m_material.centerX - m_material.width * 0.5f,
        m_material.centerY - m_material.depth * 0.5f,
        m_material.topZ - m_material.thickness,
    };
    const cadm::Vec3 size{m_material.width, m_material.depth, m_material.thickness};
    constexpr cadm::Vec3 color{0.62f, 0.62f, 0.62f};

    const auto setCommon = [&](const cadg::ShaderProgram& program) {
        const auto glBoundBorrow = program.borrowWithGlBound(*m_gl);
        glBoundBorrow.bind();
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uCameraPos", camPos));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniformMatrix4f("uViewProj", viewProjection));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1i("uHeights", 0));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform2i("uSize", nx, ny));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uMin", minimum));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uWorldSize", size));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uColor", color));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1i("uFlatShading", flatShading ? 1 : 0));
    };

    m_gl->glActiveTexture(GL_TEXTURE0);
    m_gl->glBindTexture(GL_TEXTURE_2D, heightMap.texture());

    setCommon(m_stockProgram);
    m_gl->glBindVertexArray(m_topVao);
    m_gl->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (nx - 1) * (ny - 1));

    if (m_sideMesh.valid()) {
        setCommon(m_sideProgram);
        m_sideMesh.draw(*m_gl);
    }

    m_gl->glBindVertexArray(0);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);
}

void CncRenderSystem::drawPaths(const cadm::Mat4& viewProjection) const {
    if (!m_pathMesh.valid()) {
        return;
    }
    m_gl->glDisable(GL_DEPTH_TEST);
    m_gl->glLineWidth(1.0f);

    const auto glBoundBorrow = m_lineProgram.borrowWithGlBound(*m_gl);
    glBoundBorrow.bind();
    SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniformMatrix4f("uViewProj", viewProjection));
    m_pathMesh.draw<GL_LINES>(*m_gl);
    m_gl->glEnable(GL_DEPTH_TEST);
}

void CncRenderSystem::drawTool(
    const Snapshot& snapshot,
    const cadm::Mat4& viewProjection,
    const cadm::Vec3& camPos,
    const bool flatShading
    ) const {
    const float radius = snapshot.tool.radius();
    if (!snapshot.hasPrograms || !(radius > 0.0f)) {
        return;
    }

    const cadm::Vec3 position = cncToScene(snapshot.toolPosition);
    const float cuttingLength = snapshot.tool.cuttingLength();

    const auto glBoundBorrow = m_toolProgram.borrowWithGlBound(*m_gl);
    glBoundBorrow.bind();
    SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniformMatrix4f("uViewProj", viewProjection));
    SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uCameraPos", camPos));
    SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1i("uFlatShading", flatShading ? 1 : 0));

    const auto drawMesh = [&](
        const cadg::GpuMesh& mesh,
        const cadm::Mat4& model,
        const cadm::Vec3& color
        ) {
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniformMatrix4f("uModel", model));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uColor", color));
        mesh.draw(*m_gl);
    };

    if (snapshot.tool.type == ToolType::ball) {
        const cadm::Vec3 sphereCenter = position + cadm::Vec3(0, radius, 0);
        const cadm::Mat4 sphereModel =
            cadm::Mat4::translation(sphereCenter)
            * cadm::Mat4::scale(cadm::Vec3(radius, radius, radius));
        drawMesh(m_sphereMesh, sphereModel, {0.60f, 0.68f, 0.80f});

        if (const float shaft = std::max(cuttingLength - radius, 0.0f);
            shaft > 0.0f) {
            const cadm::Mat4 shaftModel =
                cadm::Mat4::translation(sphereCenter)
                * cadm::Mat4::scale(cadm::Vec3(radius, shaft, radius))
                * cadm::Mat4::translation(cadm::Vec3(0, 0.5f, 0));
            drawMesh(m_cylinderMesh, shaftModel, {0.42f, 0.45f, 0.50f});
        }
    } else {
        const cadm::Mat4 model =
            cadm::Mat4::translation(position)
            * cadm::Mat4::scale(cadm::Vec3(radius, cuttingLength, radius))
            * cadm::Mat4::translation(cadm::Vec3(0, 0.5f, 0));
        drawMesh(m_cylinderMesh, model, {0.58f, 0.62f, 0.70f});
    }

    m_gl->glBindVertexArray(0);
}

void CncRenderSystem::render(
    const gpu::GpuHeightMap& heightMap,
    const Snapshot& snapshot,
    const cadm::Mat4& view,
    const cadm::Mat4& projection,
    const cadm::Vec3& camPos,
    const RenderOptions& options
    ) const {
    if (!m_initialized || !heightMap.initialized()) {
        return;
    }

    const auto glStateSnapshot = cadg::GlStateSnapshot::create(m_gl);

    const cadm::Mat4 viewProjection = projection * view;

    m_gl->glEnable(GL_DEPTH_TEST);
    m_gl->glDepthMask(GL_TRUE);
    m_gl->glDepthFunc(GL_LESS);
    m_gl->glDisable(GL_CULL_FACE);
    m_gl->glDisable(GL_BLEND);

    drawStock(heightMap, viewProjection, camPos, options.flatShading);
    if (options.showPaths) {
        drawPaths(viewProjection);
    }
    if (options.showTool) {
        drawTool(snapshot, viewProjection, camPos, options.flatShading);
    }

    glStateSnapshot.restore(m_gl);
}
}
