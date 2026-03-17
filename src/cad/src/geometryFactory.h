//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRYFACTORY_H
#define CAD_GEOMETRYFACTORY_H

#include "scene.h"
#include <cad_math/vec3.h>

class GeometryFactory final
{
public:
    explicit GeometryFactory(Scene &scene)
        : m_scene(scene)
    {
    }

    entity* createTorus(
        float majorRadius,
        float minorRadius,
        int majorSegments,
        int minorSegments,
        const cadm::vec3 &position = {0, 0, 0},
        const std::string &name = "Torus") const;

    entity* createAxis(
        float length = 5.0f,
        const cadm::vec3 &position = {0, 0, 0},
        const std::string &name = "Axes") const;

    entity* createGrid(
        float size = 10.0f,
        int divisions = 10,
        const cadm::vec3 &position = {0, 0, 0},
        const std::string &name = "Grid") const;

private:
    Scene &m_scene;
};

#endif //CAD_GEOMETRYFACTORY_H