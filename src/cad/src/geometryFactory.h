//
// Created on 3/15/26.
//

#ifndef CAD_GEOMETRYFACTORY_H
#define CAD_GEOMETRYFACTORY_H

#include "scene.h"
#include <cad_math/vec3.h>

class GeometryFactory
{
public:
    static entity* createTorus(
        Scene& scene, float majorRadius, float minorRadius, int majorSegments, int minorSegments,
        const cadm::vec3& position = {0, 0, 0}, const std::string& name = "Torus");

    static entity* createAxis(
        Scene& scene, float length = 5.0f, const cadm::vec3& position = {0, 0, 0}, const std::string& name = "Axes");

    static entity* createGrid(
        Scene& scene, float size = 10.0f, int divisions = 10, const cadm::vec3& position = {0, 0, 0},
        const std::string& name = "Grid");
};

#endif //CAD_GEOMETRYFACTORY_H
