//
// Created on 5/5/26.
//

#ifndef CAD_BEZIERC2COMPONENT_HPP
#define CAD_BEZIERC2COMPONENT_HPP

#include <limits>
#include <map>
#include <vector>

#include "GeometryComponent.hpp"
#include "GpuBuffer.hpp"
#include "PointRegistry.hpp"
#include <cad_math/Vec3.hpp>

#include "INewPointsTargetComponent.hpp"

/// @brief Multi-segment cubic Bézier curve with C2 continuity between segments
///
/// Can be manipulated by manipulating de Boor points or Bernstein control points. 
/// The Bernstein control points are virtual and are not part of the scene
///
/// Uses uniform parametrization (equal knot spacing)
///
/// for n de Boor points (n >= 4) there are n-3 cubic segments.
/// Segment i uses de Boor points d_i...d_{i+3}
class BezierC2Component final : public GeometryComponent,
                                public INewPointsTargetComponent<BezierC2Component> {
public:
    explicit BezierC2Component(PointRegistry *registry);

    ~BezierC2Component() override;

    /// @brief Add de Boor point to the curve
    /// @param handle handle of the point to be added
    void addControlPoint(PointHandle handle) override;

    /// @brief Remove de Boor point from the curve
    /// @param index index of the point to be removed
    void removeControlPointAt(int index);

    /// @brief Remove de Boor point from the curve
    /// @param handle handle of the point to be removed
    void removeControlPoint(PointHandle handle) override;

    [[nodiscard]] const std::vector<PointHandle>& getDeBoorPoints() const {
        return m_deBoorPoints;
    }

    [[nodiscard]] bool getShowDeBoorPolygon() const {
        return m_showDeBoorPolygon;
    }

    /// @brief Generic alias used by the shared render path
    [[nodiscard]] bool getShowControlPolyline() const {
        return m_showDeBoorPolygon;
    }

    void setShowDeBoorPolygon(bool v);

    [[nodiscard]] bool getShowBernsteinPolygon() const {
        return m_showBernsteinPolygon;
    }

    void setShowBernsteinPolygon(bool v);

    [[nodiscard]] bool getShowBernsteinCps() const {
        return m_showBernsteinCps;
    }

    void setShowBernsteinCps(bool v);

    /// @brief Move the Bernstein control point at bernsteinIndex to newPos
    /// @details Back-computes the affected de Boor point in the registry. 
    /// Inner points b1/b2 move one neighboring de Boor point;
    /// joint points move the central de Boor point
    void setBernsteinPosition(int bernsteinIndex, cadm::Vec3 newPos) const;

    [[nodiscard]] int segmentCount() const;

    void regenerateMesh() override;

    void syncToGpu() override;

    [[nodiscard]] GLuint getPatchVao() const {
        return m_patchVao;
    }

    [[nodiscard]] GLuint getDeBoorVao() const {
        return m_deBoorVao;
    }

    /// @brief Generic alias used by the shared render path
    [[nodiscard]] GLuint getControlPolylineVao() const {
        return m_deBoorVao;
    }

    [[nodiscard]] GLuint getBernsteinPolyVao() const {
        return m_bernsteinPolyVao;
    }

    [[nodiscard]] int getPatchIndexCount() const {
        return m_patchEbo.size();
    }

    [[nodiscard]] int getDeBoorIndexCount() const {
        return m_deBoorEbo.size();
    }

    /// @brief Generic alias used by the shared render path
    [[nodiscard]] int getControlPolylineIndexCount() const {
        return m_deBoorEbo.size();
    }

    [[nodiscard]] const std::vector<cadm::Vec3>& getBernsteinPositions() const {
        return m_bernsteinVbo.data();
    }

    /// @brief Recompute the Bernstein CPU cache now if a recompute is pending
    /// @details The cache is normally refreshed lazily at render time inside
    /// syncToGpu(); call this before reading getBernsteinPositions() outside the
    /// render loop to avoid observing a stale, one-edit-behind value
    void ensureBernsteinUpToDate();

private:
    /// @brief Patch VAO
    /// @details Binds m_bernsteinVBO, 
    /// EBO holds 4 indices per segment
    GLuint m_patchVao = 0;

    /// @brief De Boor polygon VAO; 
    /// @details Binds PointRegistry position VBO, 
    /// EBO holds de Boor PointHandle indices
    GLuint m_deBoorVao = 0;

    /// @brief Bernstein polygon VAO
    /// @details binds m_bernsteinVBO, 
    /// EBO = same patch indices
    GLuint m_bernsteinPolyVao = 0;

    PointRegistry *m_registry;
    std::vector<PointHandle> m_deBoorPoints;
    std::map<PointHandle, CallbackId> m_removeControlPointCallbacks;
    CallbackId m_positionCallbackId = -1;

    bool m_showDeBoorPolygon = true;
    bool m_showBernsteinPolygon = false;
    bool m_showBernsteinCps = true;

    /// @brief Set when the segment count / handles changed: forces a full
    /// Bernstein recompute and EBO rebuild on the next sync
    bool m_structureDirty = false;

    /// @brief Inclusive range of segments whose Bernstein points need recompute after a
    /// de Boor point moved. Empty when lo > hi; geometry-only
    int m_dirtySegLo = std::numeric_limits<int>::max();
    int m_dirtySegHi = -1;

    /// @brief Bernstein positions
    GpuBuffer<cadm::Vec3, GL_ARRAY_BUFFER> m_bernsteinVbo;

    /// @brief 4 per segment: {3k, 3k+1, 3k+2, 3k+3}
    /// Shared ones duplicated for the tesselation shader
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_patchEbo;

    /// @brief PointHandle indices for de Boor polygon
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_deBoorEbo;

    void removeAssociatedCallback(PointHandle h);

    /// @brief Flag a structural change:
    /// the next sync rebuilds the Bernstein cache and the EBOs
    void markStructureDirty();

    /// @brief Expand the pending dirty segment range to include [@p firstSeg, @p lastSeg]
    void markSegmentsDirty(int firstSeg, int lastSeg);

    /// @brief Mark the segments affected by de Boor point @p deBoorIndex moving: segment s
    /// is built from d[s...s+3], so moving d[index] touches segments [index-3, index]
    void markDeBoorDirty(int deBoorIndex);

    /// @brief Whether a Bernstein recompute is pending
    [[nodiscard]] bool hasDirtyBernstein() const {
        return m_structureDirty || m_dirtySegHi >= m_dirtySegLo;
    }

    void recomputeBernstein();

    void setupPatchVao(QOpenGLFunctions_4_5_Core *gl);

    void setupDeBoorVao(QOpenGLFunctions_4_5_Core *gl);

    void setupBernsteinPolyVao(QOpenGLFunctions_4_5_Core *gl);
};

#endif //CAD_BEZIERC2COMPONENT_HPP
