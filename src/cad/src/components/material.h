//
// Created on 3/15/26.
//

#ifndef CAD_MATERIAL_H
#define CAD_MATERIAL_H

#include "../entities/entity.h"
#include <cad_math/vec3.h>

class MaterialComponent : public Component
{
public:
    cadm::vec3 color{1.0f, 1.0f, 1.0f};
    float opacity = 1.0f;
    bool wireframe = false;
    float lineWidth = 1.0f;

    bool highlighted = false;
    cadm::vec3 highlightColor{1.0f, 0.5f, 0.0f};
};

#endif //CAD_MATERIAL_H
