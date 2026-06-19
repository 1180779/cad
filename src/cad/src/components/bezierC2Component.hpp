//
// Created on 5/5/26.
//

#ifndef CAD_BEZIERC2COMPONENT_HPP
#define CAD_BEZIERC2COMPONENT_HPP

#include <map>
#include <vector>

#include "GeometryComponent.hpp"
#include "GpuBuffer.hpp"
#include "PointRegistry.hpp"
#include <cad_math/vec3.hpp>

#include "INewPointsTargetComponent.hpp"
#include "../BSplineToBezierConverter.hpp"

/// Multi-segment cubic Bézier curve with C2 continuity between segments.
///
/// Can be manipulated by manipulating De Boor points or Bernstein control points. 
/// The Bernstein control points are virtual and are not part of the scene.
///
/// For supported parametrization modes see ParametrizationMode.
///
/// for n de Boor points (n >= 4) there are n-3 cubic segments.
/// Segment i uses de Boor points d_i...d_{i+3}.
class BezierC2Component final : public GeometryComponent,
                                public INewPointsTargetComponent<BezierC2Component> {
public:
    explicit BezierC2Component(PointRegistry *registry);

    ~BezierC2Component() override;

    /// Add De Boor point to the curve
    /// @param handle handle of the point to be added
    void addControlPoint(PointHandle handle) override;

    /// Remove De Boor point from the curve
    /// @param index index of the point to be removed
    void removeControlPointAt(int index);

    /// Remove De Boor point from the curve
    /// @param handle handle of the point to be removed
    void removeControlPoint(PointHandle handle);

    [[nodiscard]] const std::vector<PointHandle>& getDeBoorPoints() const {
        return m_deBoorPoints;
    }

    [[nodiscard]] bool getShowDeBoorPolygon() const {
        return m_showDeBoorPolygon;
    }

    void setShowDeBoorPolygon(bool v);

    [[nodiscard]] bool getShowBernsteinPolygon() const {
        return m_showBernsteinPolygon;
    }

    void setShowBernsteinPolygon(bool v);

    [[nodiscard]] ParametrizationMode getParametrizationMode() const {
        return m_parametrizationMode;
    }

    void setParametrizationMode(ParametrizationMode mode);

    /// @brief move the Bernstein control point at bernsteinIndex to newPos.
    /// @details 
    /// Back-computes the affected de Boor point(s) in the registry 
    /// and marks the Bernstein cache dirty
    void setBernsteinPosition(int bernsteinIndex, cadm::vec3 newPos);

    [[nodiscard]] int segmentCount() const;

    void regenerateMesh() override;

    void syncToGpu() override;

    [[nodiscard]] GLuint getPatchVao() const {
        return m_patchVao;
    }

    [[nodiscard]] GLuint getDeBoorVao() const {
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

    [[nodiscard]] const std::vector<cadm::vec3>& getBernsteinPositions() const {
        return m_bernsteinPositions;
    }

private:
    /// Patch VAO; binds m_bernsteinVBO (computed positions), EBO holds sequential indices
    GLuint m_patchVao = 0;

    /// De Boor polygon VAO; binds PointRegistry position VBO, EBO holds de Boor PointHandle indices
    GLuint m_deBoorVao = 0;

    /// Bernstein polygon VAO; binds m_bernsteinVBO, EBO = same sequential indices as a patch
    GLuint m_bernsteinPolyVao = 0;

    PointRegistry *m_registry;
    std::vector<PointHandle> m_deBoorPoints;
    std::map<PointHandle, CallbackId> m_removeControlPointCallbacks;
    CallbackId m_positionCallbackId = -1;

    bool m_showDeBoorPolygon = true;
    bool m_showBernsteinPolygon = false;
    bool m_bernsteinDirty = false;
    ParametrizationMode m_parametrizationMode = ParametrizationMode::chordLength;

    std::vector<cadm::vec3> m_bernsteinPositions; // CPU cache of computed Bernstein points
    GpuBuffer<cadm::vec3, GL_ARRAY_BUFFER> m_bernsteinVbo; // GPU: computed Bernstein positions
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_patchEbo; // sequential 0, 1, 2, 3, ...
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_deBoorEbo; // PointHandle indices for de Boor polygon

    void removeAssociatedCallback(PointHandle h);

    void recomputeBernstein();

    void setupPatchVao(QOpenGLFunctions_4_5_Core *gl);

    void setupDeBoorVao(QOpenGLFunctions_4_5_Core *gl);

    void setupBernsteinPolyVao(QOpenGLFunctions_4_5_Core *gl);
};

#endif //CAD_BEZIERC2COMPONENT_HPP
