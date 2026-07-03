//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRYFACTORY_H
#define CAD_GEOMETRYFACTORY_H

#include "Scene.hpp"
#include "PointRegistry.hpp"
#include <cad_math/Vec3.hpp>
#include <vector>

#include "PatchGeometry.hxx"

class GeometryFactory final {
public:
    explicit GeometryFactory(Scene &scene) : m_scene(scene) {}

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

    /// @brief Create a joined Bézier patch: generates all control points and the patch entity
    /// @return All created entities, control points first and the patch entity last
    [[nodiscard]] std::vector<Entity*> createPatch(const patchgen::PatchCreateParams &params) const;

private:
    Scene &m_scene;
};

#endif //CAD_GEOMETRYFACTORY_H
