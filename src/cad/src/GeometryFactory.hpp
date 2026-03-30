//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRYFACTORY_H
#define CAD_GEOMETRYFACTORY_H

#include "Scene.hpp"
#include <cad_math/vec3.hpp>

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

private:
    Scene &m_scene;
};

#endif //CAD_GEOMETRYFACTORY_H