//
// Created on 3/17/26.
//

#ifndef CAD_CAMERAFACTORY_HPP
#define CAD_CAMERAFACTORY_HPP
#include "Scene.hpp"
#include "cad_math/Vec3.hpp"
#include "components/Entity.hpp"

class CameraFactory final {
public:
    explicit CameraFactory(Scene &scene) : m_scene(scene) {}

    [[nodiscard]] Entity* createBlenderCamera(
        cadm::cadf radius,
        cadm::Vec3 target,
        const std::string &name = "On Sphere Camera"
    )
    const;

    [[nodiscard]] Entity* createCadCamera(
        const cadm::Vec3 &position,
        const cadm::Vec3 &target,
        const cadm::Vec3 &worldUp,
        const std::string &name = "Cad Camera"
    )
    const;

private:
    Scene &m_scene;
};

#endif //CAD_CAMERAFACTORY_HPP
