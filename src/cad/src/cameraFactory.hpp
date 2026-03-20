//
// Created on 3/17/26.
//

#ifndef CAD_CAMERAFACTORY_HPP
#define CAD_CAMERAFACTORY_HPP
#include "scene.h"
#include "cad_math/vec3.h"
#include "entities/entity.h"


class CameraFactory final
{
public:
    explicit CameraFactory(Scene &scene)
        : m_scene(scene)
    {
    }

    [[nodiscard]] entity* createArcBallCamera(
        cadm::cadf radius,
        cadm::vec3 target,
        cadm::vec3 worldUp,
        const std::string &name = "Camera"
    )
    const;

private:
    Scene &m_scene;
};


#endif //CAD_CAMERAFACTORY_HPP