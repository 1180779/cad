//
// Created on 4/18/26.
//

#ifndef CAD_BEZIERC0COMPONENT_HPP
#define CAD_BEZIERC0COMPONENT_HPP

#include <vector>

#include "GeometryComponent.hpp"
#include "PointRegistry.hpp"
#include "GpuBuffer.hpp"
#include "INewPointsTargetComponent.hpp"

/// Multi-segment cubic Bézier curve with C0 continuity between segments.
/// Control points are shared point entities referenced by PointHandle.
class BezierC0Component final : public GeometryComponent,
                                public INewPointsTargetComponent<BezierC0Component> {
public:
    explicit BezierC0Component(PointRegistry *registry);

    ~BezierC0Component() override;

    void addControlPoint(PointHandle h) override;

    void removeControlPointAt(int index);

    void removeControlPoint(PointHandle h);

    [[nodiscard]] const std::vector<PointHandle>& getControlPoints() const { return m_controlPoints; }
    [[nodiscard]] bool getShowPolygon() const { return m_showPolygon; }

    void setShowPolygon(bool v);

    [[nodiscard]] int segmentCount() const;

    [[nodiscard]] GLuint getPatchVao() const { return m_patchVao; }
    [[nodiscard]] GLuint getPolygonVao() const { return m_polygonVao; }

    /// Get the number of edges of the trailing segment
    /// @return 0 = no trailing, 1 = linear, 2 = quadratic
    [[nodiscard]] int trailingEdges() const;

    [[nodiscard]] int getPatchIndexCount() const { return m_patchIndexBuf.size(); }
    [[nodiscard]] int getPolygonIndexCount() const { return m_polygonIndexBuf.size(); }

    /// Rebuild EBO index lists from the current control point list (CPU only)
    void regenerateMesh() override;

    /// Upload dirty EBO data to GPU
    /// @note lazily creates VAOs on the first call
    void syncToGpu() override;

private:
    /// Patch VAO; binds PointRegistry's position VBO, EBO holds PointHandle patch sequences
    uint32_t m_patchVao = 0;

    /// Polygon VAO; binds PointRegistry's position VBO, EBO holds PointHandle line pairs
    uint32_t m_polygonVao = 0;

    PointRegistry *m_registry;
    std::vector<PointHandle> m_controlPoints;
    std::map<PointHandle, CallbackId> m_removeControlPointCallbacks;
    bool m_showPolygon = false;

    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_patchIndexBuf;
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_polygonIndexBuf;

    void removeAssociatedCallback(PointHandle h);

    void removeLastPointIncremental();

    void removeMidPointPartial(int removedIndex);

    void rebuildPatchIndices();

    void rebuildPolygonLines();

    void setupPatchVao(QOpenGLFunctions_4_5_Core *gl);

    void setupPolygonVao(QOpenGLFunctions_4_5_Core *gl);
};

#endif //CAD_BEZIERC0COMPONENT_HPP
