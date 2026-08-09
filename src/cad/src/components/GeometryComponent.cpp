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

void GeometryComponent::updateIfNecessary() {
    if (!m_needsUpdate) {
        return;
    }
    regenerateMesh();
    syncToGpu();
    m_needsUpdate = false;
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

TorusComponent::TorusComponent() {
    regenerateMesh();
}

cadm::Vec3 TorusComponent::evaluateAtAngles(const cadm::cadf majorAngle, const cadm::cadf minorAngle) const {
    const cadm::Vec3 majorRadiusPosition{
        m_majorRadius * std::cos(majorAngle),
        0,
        m_majorRadius * std::sin(majorAngle),
    };
    return {
        majorRadiusPosition.x + std::cos(majorAngle) * std::cos(minorAngle) * m_minorRadius,
        std::sin(minorAngle) * m_minorRadius,
        majorRadiusPosition.z + std::sin(majorAngle) * std::cos(minorAngle) * m_minorRadius,
    };
}

cadm::Vec3 TorusComponent::evaluateAtUv(const cadm::cadf u, const cadm::cadf v) const {
    const auto majorAngle = u * 2.0 * std::numbers::pi_v<cadm::cadf>;
    const auto minorAngle = v * 2.0 * std::numbers::pi_v<cadm::cadf>;
    return evaluateAtAngles(majorAngle, minorAngle);
}

cadm::Vec3 TorusComponent::evaluateAtUv(const cadm::Vec2 uv) const {
    return evaluateAtUv(uv.x, uv.y);
}

void TorusComponent::setMajorRadius(const cadm::cadf majorRadius) {
    if (m_majorRadius == majorRadius) {
        return;
    }
    m_majorRadius = majorRadius;
    m_needsUpdate = true;
    emit majorRadiusChanged(m_majorRadius);
}

void TorusComponent::setMinorRadius(const cadm::cadf minorRadius) {
    if (m_minorRadius == minorRadius) {
        return;
    }
    m_minorRadius = minorRadius;
    m_needsUpdate = true;
    emit minorRadiusChanged(m_minorRadius);
}

void TorusComponent::setMajorSegments(const uint32_t majorSegments) {
    if (m_majorSegments == majorSegments) {
        return;
    }
    m_majorSegments = majorSegments;
    m_needsUpdate = true;
    emit majorSegmentsChanged(static_cast<int>(m_majorSegments));
}

void TorusComponent::setMinorSegments(const uint32_t minorSegments) {
    if (m_minorSegments == minorSegments) {
        return;
    }
    m_minorSegments = minorSegments;
    m_needsUpdate = true;
    emit minorSegmentsChanged(static_cast<int>(m_minorSegments));
}

void TorusComponent::regenerateMesh() {
    auto vertices = generateVertices();
    auto indices = generateIndicesForWireframe();
    m_lineIndices.swap(indices);
    m_vertices.swap(vertices);
    m_needsUpdate = true;
}

std::vector<Vertex> TorusComponent::generateVertices() const {
    std::vector<Vertex> vertices;
    const auto majorAngleStep = static_cast<cadm::cadf>(2 * std::numbers::pi / m_majorSegments);
    const auto minorAngleStep = static_cast<cadm::cadf>(2 * std::numbers::pi / m_minorSegments);

    for (std::size_t i = 0; i < m_majorSegments; ++i) {
        const cadm::cadf majorAngle = static_cast<cadm::cadf>(i) * majorAngleStep;
        for (std::size_t j = 0; j < m_minorSegments; ++j) {
            const cadm::cadf minorAngle = static_cast<cadm::cadf>(j) * minorAngleStep;
            vertices.push_back({evaluateAtAngles(majorAngle, minorAngle), {}, {0, 0, 0, 1}});
        }
    }
    return vertices;
}

std::vector<std::uint32_t> TorusComponent::generateIndicesForWireframe() const {
    std::vector<std::uint32_t> indices;
    const auto kept = [this](const std::uint32_t i, const std::uint32_t j) {
        return m_trim.keeps(
            static_cast<cadm::cadf>(i) / static_cast<cadm::cadf>(m_majorSegments),
            static_cast<cadm::cadf>(j) / static_cast<cadm::cadf>(m_minorSegments)
        );
    };
    for (std::uint32_t i = 0; i < m_majorSegments; ++i) {
        for (std::uint32_t j = 0; j < m_minorSegments; ++j) {
            const std::uint32_t currentVertexIdx = i * m_minorSegments + j;
            if (!kept(i, j)) {
                continue;
            }

            // connect to the next vertex along the minor ring
            if (const std::uint32_t nextMinor = (j + 1) % m_minorSegments;
                kept(i, nextMinor)) {
                indices.push_back(currentVertexIdx);
                indices.push_back(i * m_minorSegments + nextMinor);
            }

            // connect to the next vertex along the major ring
            if (const std::uint32_t nextMajor = (i + 1) % m_majorSegments;
                kept(nextMajor, j)) {
                indices.push_back(currentVertexIdx);
                indices.push_back(nextMajor * m_minorSegments + j);
            }
        }
    }
    return indices;
}

void AxesComponent::regenerateMesh() {
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
