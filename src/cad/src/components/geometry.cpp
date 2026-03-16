//
// Created on 3/15/26.
//

#include "geometry.h"

#include "../checkMacros.hpp"
#include <QDebug>

TorusGeometry::TorusGeometry()
{
    m_drawMode = GL_LINES;

    const auto gl = GL();
    GLuint buffers[2];
    gl->glGenBuffers(2, buffers);
    m_VBO = buffers[0];
    m_EBO = buffers[1];
    gl->glGenVertexArrays(1, &m_VAO);

    TorusGeometry::generateMesh();
}

TorusGeometry::~TorusGeometry()
{
    const auto gl = GL();
    const GLuint buffers[2] = {m_VBO, m_EBO};
    gl->glDeleteBuffers(2, buffers);
    gl->glDeleteVertexArrays(1, &m_VAO);
}

void TorusGeometry::generateMesh()
{
    auto vertices = generateVertices();
    auto indices = generateIndicesForWireframe();
    m_indices.swap(indices);
    m_vertices.swap(vertices);
    syncMeshToGpu();
}

std::vector<Vertex> TorusGeometry::generateVertices() const
{
    std::vector<Vertex> vertices;
    const auto majorAngleStep = static_cast<cadm::cadf>(2 * std::numbers::pi / m_majorSegments);
    const auto minorAngleStep = static_cast<cadm::cadf>(2 * std::numbers::pi / m_minorSegments);

    for (std::size_t i = 0; i < m_majorSegments; ++i)
    {
        const cadm::cadf majorAngle = static_cast<cadm::cadf>(i) * majorAngleStep;
        for (std::size_t j = 0; j < m_minorSegments; ++j)
        {
            const cadm::cadf minorAngle = static_cast<cadm::cadf>(j) * minorAngleStep;

            const cadm::vec3 majorRadiusPosition{
                m_majorRadius * std::cos(majorAngle),
                m_majorRadius * std::sin(majorAngle),
                0,
            };
            const cadm::vec3 pos{
                majorRadiusPosition.x + std::cos(majorAngle) * std::cos(minorAngle) * m_minorRadius,
                majorRadiusPosition.y + std::sin(majorAngle) * std::cos(minorAngle) * m_minorRadius,
                std::sin(minorAngle) * m_minorRadius,
            };
            vertices.push_back({pos});
        }
    }
    return vertices;
}

std::vector<std::uint32_t> TorusGeometry::generateIndicesForWireframe() const
{
    std::vector<std::uint32_t> indices;
    for (std::uint32_t i = 0; i < m_majorSegments; ++i)
    {
        for (std::uint32_t j = 0; j < m_minorSegments; ++j)
        {
            const std::uint32_t currentVertexIdx = i * m_minorSegments + j;

            // connect to the next vertex along the minor ring
            const std::uint32_t nextMinorVertexIdx = i * m_minorSegments + (j + 1) % m_minorSegments;
            indices.push_back(currentVertexIdx);
            indices.push_back(nextMinorVertexIdx);

            // connect to the next vertex along the major ring
            const std::uint32_t nextMajorVertexIdx = (i + 1) % m_majorSegments * m_minorSegments + j;
            indices.push_back(currentVertexIdx);
            indices.push_back(nextMajorVertexIdx);
        }
    }
    return indices;
}

void TorusGeometry::syncMeshToGpu() const
{
    const auto gl = GL();
    gl->glBindVertexArray(m_VAO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    gl->glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
        m_vertices.data(),
        GL_STATIC_DRAW);
    gl->glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_indices.size() * sizeof(uint32_t)),
        m_indices.data(),
        GL_STATIC_DRAW);

    static_assert(std::is_same_v<cadm::vec3::VT, float> || std::is_same_v<cadm::vec3::VT, double>);
    constexpr GLenum type = std::is_same_v<cadm::vec3::VT, float>
                                ? GL_FLOAT
                                : GL_DOUBLE;
    constexpr GLsizei singleSizeof = sizeof(cadm::vec3::VT);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, type, GL_FALSE, 3 * singleSizeof, nullptr);

    GET_GL_ERRORS();
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}