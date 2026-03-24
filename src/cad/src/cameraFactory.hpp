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

    [[nodiscard]] entity* createCameraOnSphere(
        cadm::cadf radius,
        cadm::vec3 target,
        const std::string &name = "On Sphere Camera"
    )
    const;

    [[nodiscard]] entity* createCadCamera(
        const cadm::vec3 &position,
        const cadm::vec3 &target,
        const cadm::vec3 &worldUp,
        const std::string &name = "Cad Camera"
    )
    const;

private:
    Scene &m_scene;
};


#endif //CAD_CAMERAFACTORY_HPP