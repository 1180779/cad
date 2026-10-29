//
// Created by Radosław Głasek on 18.10.2026
//

#include "cnc_gpu/GpuHeightMap.hxx"

#include "cad_graphics/Gl.hxx"
#include "cad_graphics/Checks.hxx"
#include "cad_graphics/ShaderProgramGlBoundBorrow.hxx"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include <QOpenGLContext>
#include <QOpenGLVersionFunctionsFactory>

namespace cnc::gpu {

GpuHeightMap::~GpuHeightMap() {
    assert(!m_initialized && "GpuHeightMap destroyed without shutdown()");
}

bool GpuHeightMap::initialize(
    const Material& material,
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
        return fail("GpuHeightMap::initialize requires a current OpenGL context.");
    }

    if (m_initialized) {
        shutdown();
    }

    m_gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(context);
    if (m_gl == nullptr || !m_gl->initializeOpenGLFunctions()) {
        return fail("OpenGL functions are not available");
    }

    m_material = material.normalized();
    if (!compileProgram(shaderDirectory, error)) {
        destroyResources();
        return false;
    }
    if (!createResources(error)) {
        destroyResources();
        return false;
    }

    m_initialized = true;
    reset();
    return true;
}

void GpuHeightMap::shutdown() {
    destroyResources();
    m_initialized = false;
    m_gl = nullptr;
}

void GpuHeightMap::setBatchSize(const std::size_t batchSize) {
    m_batchSize = std::clamp<std::size_t>(batchSize, gc_minBatchSize, gc_maxBatchSize);
}

bool GpuHeightMap::compileProgram(
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
    auto program = cadg::ShaderProgram::fromFiles(m_gl, directory / "heightmap.vert", directory / "heightmap.frag");
    if (!program.has_value()) {
        return fail(
            "GpuHeighMap shader compilation failed:  \"" + program.error() + "\"."
            );
    }
    m_program = std::move(program.value());
    return true;
}

bool GpuHeightMap::createResources(std::string* error) {
    const auto fail = [error](std::string message) {
        if (error != nullptr) {
            *error = std::move(message);
        }
        return false;
    };

    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();

    m_gl->glGenTextures(2, m_textures);
    m_gl->glGenFramebuffers(2, m_fbos);
    for (int i = 0; i < 2; ++i) {
        m_gl->glBindTexture(GL_TEXTURE_2D, m_textures[i]);
        m_gl->glTexImage2D(GL_TEXTURE_2D, 0,GL_R32F, nx, ny, 0,GL_RED,GL_FLOAT, nullptr);
        m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbos[i]);
        m_gl->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, m_textures[i], 0);
        if (m_gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_gl->glBindTexture(GL_TEXTURE_2D, 0);
            return fail("Could not create a complete height-map framebuffer.");
        }
    }
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);

    m_gl->glGenVertexArrays(1, &m_vao);
    return true;
}

void GpuHeightMap::destroyResources() {
    if (m_gl == nullptr) {
        return;
    }
    m_program.destroy(m_gl);
    if (m_textures[0] != 0 || m_textures[1] != 0) {
        m_gl->glDeleteTextures(2, m_textures);
    }
    if (m_fbos[0] != 0 || m_fbos[1] != 0) {
        m_gl->glDeleteFramebuffers(2, m_fbos);
    }
    if (m_vao != 0) {
        m_gl->glDeleteVertexArrays(1, &m_vao);
    }
    m_textures[0] = m_textures[1] = 0;
    m_fbos[0] = m_fbos[1] = 0;
    m_vao = 0;
    m_front = 0;
}

void GpuHeightMap::reset() {
    if (!m_initialized) {
        return;
    }
    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();
    for (const unsigned int fbo : m_fbos) {
        m_gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        m_gl->glViewport(0, 0, nx, ny);
        m_gl->glClearColor(m_material.topZ, 0.0f, 0.0f, 0.0f);
        m_gl->glClear(GL_COLOR_BUFFER_BIT);
    }
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_front = 0;
}

std::size_t GpuHeightMap::apply(
    const ToolPath& moves,
    const std::size_t first,
    const std::size_t count
    ) {
    if (!m_initialized || count == 0 || first >= moves.size()) {
        return 0;
    }
    const std::size_t end = std::min(first + count, moves.size());
    std::size_t consumed = 0;
    for (std::size_t start = first; start < end; start += m_batchSize) {
        const std::size_t batch = std::min(m_batchSize, end - start);
        consumed += applyBatch(moves, start, batch);
    }
    return consumed;
}

std::size_t GpuHeightMap::applyAll(const ToolPath& moves) {
    return apply(moves, 0, moves.size());
}

bool GpuHeightMap::computeBounds(
    const Move& move,
    int& i0,
    int& i1,
    int& j0,
    int& j1
    ) const {
    const float radius = move.tool.radius();
    if (move.type == MoveType::G00 || !(radius > 0.0f)) {
        return false;
    }

    const float minX = m_material.centerX - m_material.width * 0.5f;
    const float minY = m_material.centerY - m_material.depth * 0.5f;
    const float maxX = m_material.centerX + m_material.width * 0.5f;
    const float maxY = m_material.centerY + m_material.depth * 0.5f;
    const float cellX = m_material.width / static_cast<float>(m_material.resolutionX() - 1);
    const float cellY = m_material.depth / static_cast<float>(m_material.resolutionY() - 1);
    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();

    const float minPx = std::min(move.from.x, move.to.x) - radius;
    const float maxPx = std::max(move.from.x, move.to.x) + radius;
    const float minPy = std::min(move.from.y, move.to.y) - radius;
    const float maxPy = std::max(move.from.y, move.to.y) + radius;
    if (maxPx < minX || minPx > maxX || maxPy < minY || minPy > maxY) {
        return false;
    }

    i0 = std::clamp(static_cast<int>(std::floor((minPx - minX) / cellX)), 0, nx - 1);
    i1 = std::clamp(static_cast<int>(std::ceil((maxPx - minX) / cellX)), 0, nx - 1);
    j0 = std::clamp(static_cast<int>(std::floor((minPy - minY) / cellY)), 0, ny - 1);
    j1 = std::clamp(static_cast<int>(std::ceil((maxPy - minY) / cellY)), 0, ny - 1);
    return i0 <= i1 && j0 <= j1;
}

std::size_t GpuHeightMap::applyBatch(
    const ToolPath& moves,
    const std::size_t first,
    const std::size_t count
    ) {
    const std::size_t end = std::min(first + count, moves.size());
    bool batchContainsValidCuttingMoves = false;
    for (std::size_t i = first; i < end; ++i) {
        if (moves[i].type == MoveType::G01 && moves[i].tool.radius() > 0.0f) {
            batchContainsValidCuttingMoves = true;
            break;
        }
    }
    if (!batchContainsValidCuttingMoves) {
        return 0;
    }

    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();

    const auto glSnapshot = cadg::GlStateSnapshot::create(m_gl);

    const auto back = m_front ^ 1u;
    m_gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbos[m_front]);
    m_gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbos[back]);
    m_gl->glBlitFramebuffer(0, 0, nx, ny, 0, 0, nx, ny, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbos[back]);
    m_gl->glViewport(0, 0, nx, ny);
    m_gl->glDisable(GL_DEPTH_TEST);
    m_gl->glDisable(GL_CULL_FACE);
    m_gl->glEnable(GL_SCISSOR_TEST);
    m_gl->glEnable(GL_BLEND);
    m_gl->glBlendEquation(GL_MIN);
    m_gl->glBlendFunc(GL_ONE, GL_ONE);

    const auto glBoundBorrow = cadg::ShaderProgramGlBoundBorrow(m_program, m_gl);
    glBoundBorrow.bind();
    m_gl->glBindVertexArray(m_vao);
    SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform2i("uStockSize", nx, ny));
    SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1f("uStockWMinX", m_material.centerX - m_material.width * 0.5f));
    SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1f("uStockWMinY", m_material.centerY - m_material.depth * 0.5f));
    SHADER_SET_UNIFORM_CHECK(
        glBoundBorrow.uniform1f("uStockCellWResolutionX", m_material.width / static_cast<float>(nx - 1)));
    SHADER_SET_UNIFORM_CHECK(
        glBoundBorrow.uniform1f("uStockCellWResolutionY", m_material.depth / static_cast<float>(ny - 1)));
    SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1f("uStockWBottom", m_material.topZ - m_material.thickness));

    std::size_t consumed = 0;
    for (std::size_t i = first; i < end; ++i) {
        const Move& move = moves[i];
        int i0 = 0;
        int i1 = 0;
        int j0 = 0;
        int j1 = 0;
        if (!computeBounds(move, i0, i1, j0, j1)) {
            continue;
        }

        const float deltaX = move.to.x - move.from.x;
        const float deltaY = move.to.y - move.from.y;
        const float lengthSquared = deltaX * deltaX + deltaY * deltaY;

        m_gl->glScissor(i0, j0, i1 - i0 + 1, j1 - j0 + 1);
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uCutterWFrom", move.from));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uCutterWTo", move.to));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform2f("uCutterDelta", deltaX, deltaY));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1f("uCutterLengthSquared", lengthSquared));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1f("uCutterRadius", move.tool.radius()));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1i("uCutterType", move.tool.type == ToolType::ball ? 0 : 1));
        m_gl->glDrawArrays(GL_TRIANGLES, 0, 3);
        ++consumed;
    }
    m_front ^= consumed > 0;

    m_gl->glDisable(GL_SCISSOR_TEST);
    m_gl->glDisable(GL_BLEND);
    m_gl->glBlendEquation(GL_FUNC_ADD);
    m_gl->glBindVertexArray(0);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBoundBorrow.unbind();
    glSnapshot.restore(m_gl);
    return consumed;
}

std::vector<float> GpuHeightMap::readHeights() const {
    return readHeights({
        .x = 0,
        .y = 0,
        .width = m_material.resolutionX(),
        .height = m_material.resolutionY()
    });
}

std::vector<float> GpuHeightMap::readHeights(const HeightRegion region) const {
    if (!m_initialized ||
        region.x < 0 ||
        region.y < 0 ||
        region.width <= 0 ||
        region.height <= 0 ||
        region.x + region.width > m_material.resolutionX() ||
        region.y + region.height > m_material.resolutionY()) {
        return {};
    }

    std::vector<float> heights(static_cast<std::size_t>(region.width) * region.height);
    m_gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbos[m_front]);
    m_gl->glPixelStorei(GL_PACK_ALIGNMENT, 1);
    m_gl->glReadPixels(region.x, region.y, region.width, region.height, GL_RED, GL_FLOAT, heights.data());
    m_gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return heights;
}

GLuint GpuHeightMap::texture() const {
    return m_textures[m_front];
}

const Material& GpuHeightMap::material() const {
    return m_material;
}

bool GpuHeightMap::initialized() const {
    return m_initialized;
}

std::size_t GpuHeightMap::batchSize() const {
    return m_batchSize;
}
}
