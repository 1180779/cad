//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRY_H
#define CAD_GEOMETRY_H

#include "../entities/entity.h"
#include "../gl.h"
#include <cad_math/vec3.h>
#include <vector>

struct Vertex
{
    cadm::vec3 position;
};

class GeometryComponent : public Component
{
public:
    virtual void generateMesh() = 0;
    ~GeometryComponent() override = default;

    std::vector<Vertex> m_vertices;
    std::vector<std::uint32_t> m_indices;

    uint32_t m_VAO = 0;
    uint32_t m_VBO = 0;
    uint32_t m_EBO = 0;
    GLenum m_drawMode = GL_TRIANGLES;

    bool m_needsUpdate = true;
};

class TorusGeometry final : public GeometryComponent
{
public:
    TorusGeometry();
    ~TorusGeometry() override;

    // [[nodiscard]] cadm::cadf getMajorRadius() const { return m_majorRadius; }
    // [[nodiscard]] cadm::cadf getMinorRadius() const { return m_minorRadius; }
    // [[nodiscard]] int getMajorSegments() const { return m_majorSegments; }
    // [[nodiscard]] int getMinorSegments() const { return m_minorSegments; }

    void generateMesh() override;
    [[nodiscard]] std::vector<Vertex> generateVertices() const;
    [[nodiscard]] std::vector<std::uint32_t> generateIndicesForWireframe() const;
    void syncMeshToGpu() const;

    cadm::cadf m_majorRadius = 2.0f;
    cadm::cadf m_minorRadius = 0.5f;
    uint32_t m_majorSegments = 48;
    uint32_t m_minorSegments = 24;
};

class AxesGeometry final : public GeometryComponent
{
public:
    cadm::cadf m_length = 5.0f;

    void generateMesh() override;
};

class GridGeometry final : public GeometryComponent
{
public:

    void generateMesh() override;
};

#endif //CAD_GEOMETRY_H
