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
            "GpuHeighMap heightmap shader compilation failed:  \"" + program.error() + "\"."
            );
    }
    m_program = std::move(program.value());
    auto issues = cadg::ShaderProgram::fromFiles(m_gl, directory / "heightmap.vert", directory / "issues.frag");
    if (!issues.has_value()) {
        return fail("GpuHeighMap issues shader compilation failed: " + issues.error());
    }
    m_issueProgram = std::move(issues.value());
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

    m_gl->glGenTextures(1, &m_texture);
    m_gl->glGenFramebuffers(1, &m_fbo);
    m_gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, nx, ny, 0,GL_RED, GL_FLOAT, nullptr);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    m_gl->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, m_texture, 0);
    if (m_gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_gl->glBindTexture(GL_TEXTURE_2D, 0);
        return fail("Could not create a complete height-map framebuffer.");
    }
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);

    m_gl->glGenFramebuffers(1, &m_issueFbo);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_issueFbo);
    m_gl->glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, nx);
    m_gl->glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, ny);
    m_gl->glDrawBuffer(GL_NONE);
    m_gl->glReadBuffer(GL_NONE);
    const bool complete = m_gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete) return fail("Could not create the issue-check framebuffer.");
    m_gl->glCreateBuffers(1, &m_issueBuffer);

    m_gl->glGenVertexArrays(1, &m_vao);
    return true;
}

void GpuHeightMap::destroyResources() {
    if (m_gl == nullptr) {
        return;
    }
    m_program.destroy(m_gl);
    m_issueProgram.destroy(m_gl);
    if (m_issueFbo != 0) {
        m_gl->glDeleteFramebuffers(1, &m_issueFbo);
        m_issueFbo = 0;
    }
    if (m_issueBuffer != 0) {
        m_gl->glDeleteBuffers(1, &m_issueBuffer);
        m_issueBuffer = 0;
    }
    if (m_texture != 0) {
        m_gl->glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    if (m_fbo != 0) {
        m_gl->glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_vao != 0) {
        m_gl->glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void GpuHeightMap::reset() {
    if (!m_initialized) {
        return;
    }
    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    m_gl->glViewport(0, 0, nx, ny);
    m_gl->glClearColor(m_material.topZ, 0.0f, 0.0f, 0.0f);
    m_gl->glClear(GL_COLOR_BUFFER_BIT);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::size_t GpuHeightMap::apply(
    const ToolPath& moves,
    const std::size_t first,
    const std::size_t count,
    std::vector<std::uint32_t>* issues
    ) {
    if (issues) issues->clear();
    if (!m_initialized || count == 0 || first >= moves.size()) {
        return 0;
    }
    const std::size_t end = first + std::min(count, moves.size() - first);
    if (issues) issues->resize(end - first, 0u);
    std::size_t consumed = 0;
    for (std::size_t start = first; start < end; start += m_batchSize) {
        const std::size_t batch = std::min(m_batchSize, end - start);
        consumed += applyBatch(moves, start, batch, issues ? issues->data() + start - first : nullptr);
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
    if (!(radius > 0.0f)) {
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
    const std::size_t count,
    std::uint32_t* issues
    ) {
    const std::size_t end = std::min(first + count, moves.size());

    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();
    const auto glSnapshot = cadg::GlStateSnapshot::create(m_gl);
    const GLboolean oldStencil = m_gl->glIsEnabled(GL_STENCIL_TEST);
    GLint oldActiveTexture{}, oldTexture{}, oldBuffer{}, oldIndexedBuffer{};
    GLint64 oldBufferStart{}, oldBufferSize{};
    if (issues) {
        m_gl->glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTexture);
        m_gl->glActiveTexture(GL_TEXTURE0);
        m_gl->glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
        m_gl->glBindTexture(GL_TEXTURE_2D, m_texture);
        m_gl->glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &oldBuffer);
        m_gl->glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 0, &oldIndexedBuffer);
        m_gl->glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_START, 0, &oldBufferStart);
        m_gl->glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_SIZE, 0, &oldBufferSize);
        m_gl->glNamedBufferData(m_issueBuffer, count * sizeof(std::uint32_t), issues, GL_STREAM_READ);
        m_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_issueBuffer);
    }

    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    m_gl->glViewport(0, 0, nx, ny);

    m_gl->glDisable(GL_DEPTH_TEST);
    m_gl->glDisable(GL_STENCIL_TEST);
    m_gl->glDisable(GL_CULL_FACE);
    m_gl->glEnable(GL_SCISSOR_TEST);
    m_gl->glEnable(GL_BLEND);
    m_gl->glBlendEquation(GL_MIN);

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
        if (issues) {
            m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_issueFbo);
            const cadg::ShaderProgramGlBoundBorrow glBoundIssue(m_issueProgram, m_gl);
            glBoundIssue.bind();
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform1i("uMoveSlot", static_cast<int>(i - first)));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform2f("uStockMin", m_material.minXY()));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform2f("uStockCellSize", m_material.cellSizeXY()));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform1f("uStockBottom", m_material.bottomZ()));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform1f("uStockTop", m_material.topZ));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform3f("uFrom", move.from));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform3f("uTo", move.to));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform1f("uRadius", move.tool.radius()));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform1f("uCuttingLength", move.tool.cuttingLength()));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform1i("uBall", move.tool.type == ToolType::ball));
            SHADER_SET_UNIFORM_CHECK(glBoundIssue.uniform1i("uRapid", move.type == MoveType::G00));
            m_gl->glDrawArrays(GL_TRIANGLES, 0, 3);
            m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
            glBoundBorrow.bind();
        }
        if (move.type == MoveType::G00) continue;
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uCutterWFrom", move.from));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform3f("uCutterWTo", move.to));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform2f("uCutterDelta", deltaX, deltaY));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1f("uCutterLengthSquared", lengthSquared));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1f("uCutterRadius", move.tool.radius()));
        SHADER_SET_UNIFORM_CHECK(glBoundBorrow.uniform1i("uCutterType", move.tool.type == ToolType::ball ? 0 : 1));
        m_gl->glDrawArrays(GL_TRIANGLES, 0, 3);
        ++consumed;
    }

    if (issues) {
        m_gl->glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        m_gl->glGetNamedBufferSubData(m_issueBuffer, 0, count * sizeof(std::uint32_t), issues);
        if (oldIndexedBuffer != 0 && oldBufferSize > 0) {
            m_gl->glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, oldIndexedBuffer, oldBufferStart, oldBufferSize);
        } else {
            m_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, oldIndexedBuffer);
        }
        m_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, oldBuffer);
        m_gl->glBindTexture(GL_TEXTURE_2D, oldTexture);
        m_gl->glActiveTexture(oldActiveTexture);
    }
    if (oldStencil) m_gl->glEnable(GL_STENCIL_TEST);
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
    if (!m_initialized) return {};
    const int nx = m_material.resolutionX();
    const int ny = m_material.resolutionY();
    std::vector<float> heights(static_cast<std::size_t>(nx) * ny);
    m_gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    m_gl->glPixelStorei(GL_PACK_ALIGNMENT, 1);
    m_gl->glReadPixels(0, 0, nx, ny, GL_RED, GL_FLOAT, heights.data());
    m_gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return heights;
}

GLuint GpuHeightMap::texture() const {
    return m_texture;
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
