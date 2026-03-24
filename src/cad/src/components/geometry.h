//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRY_H
#define CAD_GEOMETRY_H

#include "../entities/entity.h"
#include "../gl.h"
#include <cad_math/vec3.h>
#include <vector>
#include <QObject>

struct Vertex
{
    cadm::vec3 position;
};

class GeometryComponent : public Component
{
public:
    virtual void regenerateMesh() = 0;

    virtual void syncToGpu()
    {
    }

    ~GeometryComponent() override = default;

    std::vector<Vertex> m_vertices;
    std::vector<std::uint32_t> m_indices;

    uint32_t m_VAO = 0;
    uint32_t m_VBO = 0;
    uint32_t m_EBO = 0;
    GLenum m_drawMode = GL_TRIANGLES;

    bool m_needsUpdate = true;
};

class TorusGeometry final : public QObject, public GeometryComponent
{
    Q_OBJECT

    Q_PROPERTY(double majorRadius READ getMajorRadius WRITE setMajorRadius NOTIFY majorRadiusChanged)
    Q_PROPERTY(double minorRadius READ getMinorRadius WRITE setMinorRadius NOTIFY minorRadiusChanged)
    Q_PROPERTY(int majorSegments READ getMajorSegments WRITE setMajorSegments NOTIFY majorSegmentsChanged)
    Q_PROPERTY(int minorSegments READ getMinorSegments WRITE setMinorSegments NOTIFY minorSegmentsChanged)

public:
    TorusGeometry();
    ~TorusGeometry() override;

    [[nodiscard]] cadm::cadf getMajorRadius() const { return m_majorRadius; }
    [[nodiscard]] cadm::cadf getMinorRadius() const { return m_minorRadius; }
    [[nodiscard]] uint32_t getMajorSegments() const { return m_majorSegments; }
    [[nodiscard]] uint32_t getMinorSegments() const { return m_minorSegments; }

    void setMajorRadius(cadm::cadf m_majorRadius);
    void setMinorRadius(cadm::cadf m_minorRadius);
    void setMajorSegments(uint32_t m_majorSegments);
    void setMinorSegments(uint32_t m_minorSegments);

    void regenerateMesh() override;
    void syncToGpu() override;
    [[nodiscard]] std::vector<Vertex> generateVertices() const;
    [[nodiscard]] std::vector<std::uint32_t> generateIndicesForWireframe() const;

private:
    cadm::cadf m_majorRadius = 2.0f;
    cadm::cadf m_minorRadius = 0.5f;
    uint32_t m_majorSegments = 48;
    uint32_t m_minorSegments = 24;

signals:
    void majorRadiusChanged(double radius);
    void minorRadiusChanged(double radius);
    void majorSegmentsChanged(int segments);
    void minorSegmentsChanged(int segments);
};

class AxesGeometry final : public GeometryComponent
{
public:
    cadm::cadf m_length = 5.0f;

    void regenerateMesh() override;
};

class GridGeometry final : public GeometryComponent
{
public:
    void regenerateMesh() override;
};

#endif //CAD_GEOMETRY_H
