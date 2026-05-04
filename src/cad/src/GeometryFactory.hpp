//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRYFACTORY_H
#define CAD_GEOMETRYFACTORY_H

#include "Scene.hpp"
#include "PointRegistry.hpp"
#include <cad_math/vec3.hpp>
#include <vector>

class GeometryFactory final
{
public:
    explicit GeometryFactory(Scene &scene)
        : m_scene(scene)
    {
    }

    Entity* createTorus(
        float majorRadius,
        float minorRadius,
        int majorSegments,
        int minorSegments,
        const cadm::vec3 &position = {0, 0, 0},
        const std::string &name = "Torus") const;

    Entity* createAxis(
        float length = 5.0f,
        const cadm::vec3 &position = {0, 0, 0},
        const std::string &name = "Axes") const;

    Entity* createCursor(
        const cadm::vec3 &position = {0, 0, 0},
        const std::string &name = "Cursor") const;

    Entity* createPoint(
        const cadm::vec3 &position = {0, 0, 0},
        const std::string &name = "Point") const;

    Entity* createBezierC0(
        const std::vector<PointHandle> &controlPoints = {},
        const std::string &name = "BezierC0") const;

private:
    Scene &m_scene;
};

#endif //CAD_GEOMETRYFACTORY_H
