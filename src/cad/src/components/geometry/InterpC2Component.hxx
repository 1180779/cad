//
// Created by Radosław Głasek on 23.06.2026
//

#ifndef CAD_INTERPC2COMPONENT_HXX
#define CAD_INTERPC2COMPONENT_HXX

#include <map>
#include <vector>

#include "../GeometryComponent.hpp"
#include "GpuBuffer.hpp"
#include "Vao.hxx"
#include "PointRegistry.hpp"
#include <cad_math/Vec3.hpp>

#include "../INewPointsTargetComponent.hpp"

/// @brief C2 cubic spline interpolating a sequence of control points
///
/// @details Passes through every control point with C2 continuity, using chord-length
/// parametrization. The interpolation is solved globally (Thomas algorithm, see InterpC2Solver)
/// and converted to piecewise cubic Bezier and shares the Bezier patch-tessellation render
/// path. The Bernstein points are derived (virtual, non-editable); editing means moving the
/// interpolated points themselves
///
/// @note Unlike other curves, moving one point perturbs the whole spline, so every change triggers a
/// full re-solve
class InterpC2Component final : public GeometryComponent,
                                public INewPointsTargetComponent<InterpC2Component> {
public:
    explicit InterpC2Component(PointRegistry *registry);

    ~InterpC2Component() override;

    void addControlPoint(PointHandle handle) override;

    void removeControlPointAt(int index);

    void removeControlPoint(PointHandle handle) override;

    [[nodiscard]] const std::vector<PointHandle>& getControlPoints() const {
        return m_points;
    }

    [[nodiscard]] bool getShowControlPolyline() const {
        return m_showPolyline;
    }

    void setShowControlPolyline(bool v);

    [[nodiscard]] bool getShowBernsteinPolygon() const {
        return m_showBernsteinPolygon;
    }

    void setShowBernsteinPolygon(bool v);

    [[nodiscard]] bool getShowBernsteinCps() const {
        return m_showBernsteinCps;
    }

    void setShowBernsteinCps(bool v);

    [[nodiscard]] int segmentCount() const {
        const int n = m_bernsteinVbo.size();
        return n > 0
                   ? (n - 1) / 3
                   : 0;
    }

    void regenerateMesh() override;

    void syncToGpu() override;

    [[nodiscard]] GLuint getPatchVao() const {
        return m_patchVao.id();
    }

    [[nodiscard]] GLuint getControlPolylineVao() const {
        return m_polylineVao.id();
    }

    [[nodiscard]] GLuint getBernsteinPolyVao() const {
        return m_bernsteinPolyVao.id();
    }

    [[nodiscard]] int getPatchIndexCount() const {
        return m_patchEbo.size();
    }

    [[nodiscard]] int getControlPolylineIndexCount() const {
        return m_polylineEbo.size();
    }

    [[nodiscard]] const std::vector<cadm::Vec3>& getBernsteinPositions() const {
        return m_bernsteinVbo.data();
    }

    /// @brief Recompute the Bernstein cache now if a re-solve is pending
    void ensureBernsteinUpToDate();

private:
    Vao m_patchVao;
    Vao m_polylineVao;
    Vao m_bernsteinPolyVao;

    PointRegistry *m_registry;
    std::vector<PointHandle> m_points;
    std::map<PointHandle, CallbackId> m_removeControlPointCallbacks;
    CallbackId m_positionCallbackId = -1;

    bool m_showPolyline = true;
    bool m_showBernsteinPolygon = false;
    bool m_showBernsteinCps = true;

    /// @brief A re-solve + EBO rebuild is pending
    bool m_dirty = false;

    GpuBuffer<cadm::Vec3, GL_ARRAY_BUFFER> m_bernsteinVbo;
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_patchEbo;
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_polylineEbo;

    void removeAssociatedCallback(PointHandle h);

    void markDirty();

    void recompute();
};

#endif //CAD_INTERPC2COMPONENT_HXX
