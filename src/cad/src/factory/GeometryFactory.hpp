//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRYFACTORY_H
#define CAD_GEOMETRYFACTORY_H

#include "../Scene.hpp"
#include "../PointRegistry.hpp"
#include <cad_math/Vec3.hpp>
#include <vector>

#include "../PatchGeometry.hxx"
#include "../utils/IntersectionUtils.hxx"

class GeometryFactory final {
public:
    explicit GeometryFactory(Scene &scene)
    : m_scene(scene) {}

    Entity* createTorus(
        float majorRadius,
        float minorRadius,
        int majorSegments,
        int minorSegments,
        const cadm::Vec3 &position = {0, 0, 0},
        const std::string &name = "Torus"
    ) const;

    Entity* createAxis(
        float length = 5.0f,
        const cadm::Vec3 &position = {0, 0, 0},
        const std::string &name = "Axes"
    ) const;

    Entity* createCursor(
        const cadm::Vec3 &position = {0, 0, 0},
        const std::string &name = "Cursor"
    ) const;

    Entity* createPoint(
        const cadm::Vec3 &position = {0, 0, 0},
        const std::string &name = "Point"
    ) const;

    Entity* createBezierC0(
        const std::vector<PointHandle> &controlPoints = {},
        const std::string &name = "BezierC0"
    ) const;

    Entity* createBezierC2(
        const std::vector<PointHandle> &controlPoints = {},
        const std::string &name = "BezierC2"
    ) const;

    Entity* createInterpC2(
        const std::vector<PointHandle> &controlPoints = {},
        const std::string &name = "InterpC2"
    ) const;

    /// @brief Create a Gregory hole fill referencing existing control points
    /// @param holeHandles flat list,
    /// <tt>GregoryComponent::s_handlesPerEdge</tt> per hole edge (boundary row
    /// then inner row, both oriented along the hole cycle);
    /// @param name Entity name
    Entity* createGregory(
        const std::vector<PointHandle> &holeHandles,
        const std::string &name = "Gregory"
    ) const;

    /// @brief Create an intersection curve entity from an already-traced curve
    /// @param patch1,patch2 ids of the source surface entities (see
    /// <tt>intersections::traceIntersectionCurve</tt>)
    /// @param curve the traced curve (params + closed flag)
    /// @param data per-surface param points and 3D points, see
    /// <tt>intersections::extractCurveData</tt>
    /// @param wrap1,wrap2 which parameters of each surface are periodic
    /// @param name Entity name
    Entity* createIntersectionCurve(
        EntityId patch1,
        EntityId patch2,
        const intersections::IntersectionCurve &curve,
        const intersections::IntersectionCurveData &data,
        trimming::SurfaceWrap wrap1 = {},
        trimming::SurfaceWrap wrap2 = {},
        const std::string &name = "Intersection"
    ) const;

    /// @brief Create a joined Bézier patch: generates all control points and the patch entity
    /// @return All created entities, control points first and the patch entity last
    [[nodiscard]] std::vector<Entity*> createPatch(const patchgen::PatchCreateParams &params) const;

    /// @brief Turn a traced polyline into an interpolating C2 spline
    /// @param points the points of the curve to interpolate
    /// @param everyNth keep only every n-th point
    /// @param name Entity name
    /// @returns All created entities, control points first and the curve last
    [[nodiscard]] std::vector<Entity*> createInterpolatedFromPoints(
        const std::vector<cadm::Vec3> &points,
        int everyNth = 10,
        const std::string &name = "Intersection Spline"
    ) const;

private:
    Scene &m_scene;
};

#endif //CAD_GEOMETRYFACTORY_H
