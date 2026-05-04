//
// Created on 3/17/26.
//

#include "CameraFactory.hpp"

#include "components/CadCameraComponent.hpp"
#include "components/BlenderCameraComponent.hpp"
#include "gui/TransformWidget.hpp"

Entity* CameraFactory::createBlenderCamera(
    const cadm::cadf radius,
    const cadm::vec3 target,
    const std::string &name) const
{
    const auto entity = m_scene.createEntity(name);
    const auto camera = entity->addComponent<BlenderCameraComponent>();
    camera->setRadius(radius);
    camera->setTarget(target);
    return entity;
}

Entity* CameraFactory::createCadCamera(
    const cadm::vec3 &position,
    const cadm::vec3 &target,
    const cadm::vec3 &worldUp,
    const std::string &name) const
{
    const auto entity = m_scene.createEntity(name);
    const auto camera = entity->addComponent<CadCameraComponent>();
    camera->setPosition(position);
    camera->setTarget(target);
    camera->setWorldUp(worldUp);
    return entity;
}