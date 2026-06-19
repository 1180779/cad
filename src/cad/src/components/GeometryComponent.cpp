//
// Created on 3/15/26.
//

#include "GeometryComponent.hpp"

#include "../CheckMacros.hpp"

GeometryComponent::~GeometryComponent() {
    if (m_VAO != 0) {
        const auto gl = getGl();
        const GLuint buffers[3] = {m_VBO, m_EBO_Lines, m_EBO_Triangles};
        gl->glDeleteBuffers(3, buffers);
        gl->glDeleteVertexArrays(1, &m_VAO);
    }
}

void GeometryComponent::syncToGpu() {
    const auto gl = getGl();

    if (m_VAO == 0) {
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
        GL_STATIC_DRAW
    );

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO_Triangles);
    gl->glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_triangleIndices.size() * sizeof(uint32_t)),
        m_triangleIndices.data(),
        GL_STATIC_DRAW
    );

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO_Lines);
    gl->glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_lineIndices.size() * sizeof(uint32_t)),
        m_lineIndices.data(),
        GL_STATIC_DRAW
    );

    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, gc_glCadmVtType, GL_FALSE, 10 * gc_glCadmVtSize, nullptr);
    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(
        1,
        3,
        gc_glCadmVtType,
        GL_FALSE,
        10 * gc_glCadmVtSize,
        reinterpret_cast<void*>(3 * gc_glCadmVtSize)
    );
    gl->glEnableVertexAttribArray(2);
    gl->glVertexAttribPointer(
        2,
        4,
        gc_glCadmVtType,
        GL_FALSE,
        10 * gc_glCadmVtSize,
        reinterpret_cast<void*>(6 * gc_glCadmVtSize)
    );

    GET_GL_ERRORS();
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

TorusGeometry::TorusGeometry() {
    regenerateMesh();
}

void TorusGeometry::setMajorRadius(const cadm::cadf majorRadius) {
    if (m_majorRadius == majorRadius) {
        return;
    }
    m_majorRadius = majorRadius;
    m_needsUpdate = true;
    emit majorRadiusChanged(m_majorRadius);
}

void TorusGeometry::setMinorRadius(const cadm::cadf minorRadius) {
    if (m_minorRadius == minorRadius) {
        return;
    }
    m_minorRadius = minorRadius;
    m_needsUpdate = true;
    emit minorRadiusChanged(m_minorRadius);
}

void TorusGeometry::setMajorSegments(const uint32_t majorSegments) {
    if (m_majorSegments == majorSegments) {
        return;
    }
    m_majorSegments = majorSegments;
    m_needsUpdate = true;
    emit majorSegmentsChanged(static_cast<int>(m_majorSegments));
}

void TorusGeometry::setMinorSegments(const uint32_t minorSegments) {
    if (m_minorSegments == minorSegments) {
        return;
    }
    m_minorSegments = minorSegments;
    m_needsUpdate = true;
    emit minorSegmentsChanged(static_cast<int>(m_minorSegments));
}

void TorusGeometry::regenerateMesh() {
    auto vertices = generateVertices();
    auto indices = generateIndicesForWireframe();
    m_lineIndices.swap(indices);
    m_vertices.swap(vertices);
    m_needsUpdate = true;
}

std::vector<Vertex> TorusGeometry::generateVertices() const {
    std::vector<Vertex> vertices;
    const auto majorAngleStep = static_cast<cadm::cadf>(2 * std::numbers::pi / m_majorSegments);
    const auto minorAngleStep = static_cast<cadm::cadf>(2 * std::numbers::pi / m_minorSegments);

    for (std::size_t i = 0; i < m_majorSegments; ++i) {
        const cadm::cadf majorAngle = static_cast<cadm::cadf>(i) * majorAngleStep;
        for (std::size_t j = 0; j < m_minorSegments; ++j) {
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
            vertices.push_back({pos, {}, {0, 0, 0, 1}});
        }
    }
    return vertices;
}

std::vector<std::uint32_t> TorusGeometry::generateIndicesForWireframe() const {
    std::vector<std::uint32_t> indices;
    for (std::uint32_t i = 0; i < m_majorSegments; ++i) {
        for (std::uint32_t j = 0; j < m_minorSegments; ++j) {
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

void AxesGeometry::regenerateMesh() {
    m_vertices.clear();
    m_lineIndices.clear();

    m_vertices.push_back({{0, 0, 0}, {}, xColor});
    m_vertices.push_back({{m_length, 0, 0}, {}, xColor});
    m_vertices.push_back({{0, 0, 0}, {}, yColor});
    m_vertices.push_back({{0, m_length, 0}, {}, yColor});
    m_vertices.push_back({{0, 0, 0}, {}, zColor});
    m_vertices.push_back({{0, 0, m_length}, {}, zColor});

    m_lineIndices = {0, 1, 2, 3, 4, 5};
}
