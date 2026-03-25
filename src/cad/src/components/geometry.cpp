//
// Created on 3/15/26.
//

#include "geometry.h"

#include "../checkMacros.hpp"
#include <QDebug>

TorusGeometry::TorusGeometry()
{
    regenerateMesh();
}

void TorusGeometry::setMajorRadius(cadm::cadf majorRadius)
{
    if (m_majorRadius == majorRadius)
        return;
    m_majorRadius = majorRadius;
    m_needsUpdate = true;
    emit majorRadiusChanged(m_majorRadius);
}

void TorusGeometry::setMinorRadius(cadm::cadf minorRadius)
{
    if (m_minorRadius == minorRadius)
        return;
    m_minorRadius = minorRadius;
    m_needsUpdate = true;
    emit minorRadiusChanged(m_minorRadius);
}

void TorusGeometry::setMajorSegments(uint32_t majorSegments)
{
    if (m_majorSegments == majorSegments)
        return;
    m_majorSegments = majorSegments;
    m_needsUpdate = true;
    emit majorSegmentsChanged(static_cast<int>(m_majorSegments));
}

void TorusGeometry::setMinorSegments(uint32_t minorSegments)
{
    if (m_minorSegments == minorSegments)
        return;
    m_minorSegments = minorSegments;
    m_needsUpdate = true;
    emit minorSegmentsChanged(static_cast<int>(m_minorSegments));
}

void TorusGeometry::syncToGpu()
{
    const auto gl = GL();

    if (m_VAO == 0)
    {
        GLuint buffers[3];
        gl->glGenBuffers(3, buffers);
        m_VBO = buffers[0];
        m_EBO_Lines = buffers[1];
        m_EBO_Triangles = buffers[2];
        gl->glGenVertexArrays(1, &m_VAO);
    }

    gl->glBindVertexArray(m_VAO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    gl->glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
        m_vertices.data(),
        GL_STATIC_DRAW);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO_Lines);
    gl->glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_lineIndices.size() * sizeof(uint32_t)),
        m_lineIndices.data(),
        GL_STATIC_DRAW);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO_Triangles);
    gl->glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_triangleIndices.size() * sizeof(uint32_t)),
        m_triangleIndices.data(),
        GL_STATIC_DRAW);

    static_assert(std::is_same_v<cadm::vec3::VT, float> || std::is_same_v<cadm::vec3::VT, double>);
    constexpr GLenum type = std::is_same_v<cadm::vec3::VT, float>
                                ? GL_FLOAT
                                : GL_DOUBLE;
    constexpr GLsizei singleSizeof = sizeof(cadm::vec3::VT);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, type, GL_FALSE, 10 * singleSizeof, nullptr);
    gl->glVertexAttribPointer(1, 3, type, GL_FALSE, 10 * singleSizeof, reinterpret_cast<void *>(3 * singleSizeof));
    gl->glVertexAttribPointer(2, 3, type, GL_FALSE, 10 * singleSizeof, reinterpret_cast<void *>(6 * singleSizeof));

    GET_GL_ERRORS();
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

TorusGeometry::~TorusGeometry()
{
    if (m_VAO != 0)
    {
        const auto gl = GL();
        const GLuint buffers[3] = {m_VBO, m_EBO_Lines, m_EBO_Triangles};
        gl->glDeleteBuffers(3, buffers);
        gl->glDeleteVertexArrays(1, &m_VAO);
    }
}

void TorusGeometry::regenerateMesh()
{
    auto vertices = generateVertices();
    auto indices = generateIndicesForWireframe();
    m_lineIndices.swap(indices);
    m_vertices.swap(vertices);
    m_needsUpdate = true;
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

void GridGeometry::regenerateMesh()
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    const auto halfSize = m_size / 2;
    const auto step = m_size / static_cast<cadm::cadf>(m_divisions);

    std::uint32_t index = 0;

    for (std::uint32_t i = 0; i <= m_divisions; ++i)
    {
        const auto pos = -halfSize + static_cast<cadm::cadf>(i) * step;
        vertices.push_back({{pos, -halfSize, 0}});
        vertices.push_back({{pos, halfSize, 0}});
        indices.push_back(index++);

    }
}
