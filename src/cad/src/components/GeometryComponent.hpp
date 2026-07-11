//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRY_H
#define CAD_GEOMETRY_H

#include "Entity.hpp"
#include <cad_math/Vec3.hpp>
#include <vector>
#include <QObject>
#include <string>

#include "cad_math/Vec2.hpp"
#include "cad_math/Vec4.hpp"

struct Vertex {
    cadm::Vec3 position;
    cadm::Vec3 normal;
    cadm::Vec4 color;
};

class GeometryComponent : public Component {
public:
    virtual void regenerateMesh() = 0;

    virtual void syncToGpu();

    ~GeometryComponent() override;

    void select() {
        m_selected = true;
    }

    /// @brief Regenerate the mesh and sync it to GPU if marked for update
    void updateIfNecessary();

    /// @brief Mark for mesh regeneration and GPU sync
    /// @note Sets @ref m_needsUpdate to true; the operations are done lazily
    void markForUpdate() {
        m_needsUpdate = true;
    }

    std::vector<Vertex> m_vertices;
    std::vector<std::uint32_t> m_triangleIndices;
    std::vector<std::uint32_t> m_lineIndices;

    uint32_t m_VAO = 0;
    uint32_t m_VBO = 0;
    uint32_t m_EBO_Triangles = 0;
    uint32_t m_EBO_Lines = 0;

    bool m_selected = false;

    /// @brief Indicates whether component regeneration and GPU sync are required
    bool m_needsUpdate = true;
};

class TorusComponent final : public QObject, public GeometryComponent {
    Q_OBJECT Q_PROPERTY(double majorRadius READ getMajorRadius WRITE setMajorRadius NOTIFY majorRadiusChanged)

    Q_PROPERTY(double minorRadius READ getMinorRadius WRITE setMinorRadius NOTIFY minorRadiusChanged)

    Q_PROPERTY(int majorSegments READ getMajorSegments WRITE setMajorSegments NOTIFY majorSegmentsChanged)

    Q_PROPERTY(int minorSegments READ getMinorSegments WRITE setMinorSegments NOTIFY minorSegmentsChanged)

public:
    TorusComponent();

    [[nodiscard]] cadm::Vec3 evaluateAtAngles(cadm::cadf majorAngle, cadm::cadf minorAngle) const;

    [[nodiscard]] cadm::Vec3 evaluateAtUv(cadm::cadf u, cadm::cadf v) const;

    [[nodiscard]] cadm::Vec3 evaluateAtUv(cadm::Vec2 uv) const;

    [[nodiscard]] cadm::cadf getMajorRadius() const {
        return m_majorRadius;
    }

    [[nodiscard]] cadm::cadf getMinorRadius() const {
        return m_minorRadius;
    }

    [[nodiscard]] uint32_t getMajorSegments() const {
        return m_majorSegments;
    }

    [[nodiscard]] uint32_t getMinorSegments() const {
        return m_minorSegments;
    }

    void setMajorRadius(cadm::cadf majorRadius);

    void setMinorRadius(cadm::cadf minorRadius);

    void setMajorSegments(uint32_t majorSegments);

    void setMinorSegments(uint32_t minorSegments);

    void regenerateMesh() override;

    [[nodiscard]] std::vector<Vertex> generateVertices() const;

    [[nodiscard]] std::vector<std::uint32_t> generateIndicesForWireframe() const;

private:
    cadm::cadf m_majorRadius = 2.0f;
    cadm::cadf m_minorRadius = 0.5f;
    uint32_t m_majorSegments = 48;
    uint32_t m_minorSegments = 24;

signals :
    void majorRadiusChanged(double radius);

    void minorRadiusChanged(double radius);

    void majorSegmentsChanged(int segments);

    void minorSegmentsChanged(int segments);
};

class AxesComponent final : public GeometryComponent {
public:
    cadm::cadf m_length = 5.0f;
    float m_lineWidth = 1.0f;
    static constexpr cadm::Vec4 xColor{1, 0, 0, 1};
    static constexpr cadm::Vec4 yColor{0, 1, 0, 1};
    static constexpr cadm::Vec4 zColor{0, 0, 1, 1};

    void regenerateMesh() override;
};

#endif //CAD_GEOMETRY_H
