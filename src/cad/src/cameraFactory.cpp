//
// Created on 3/17/26.
//

#include "cameraFactory.hpp"

#include "components/cadCameraCompoonent.hpp"
#include "components/camera.hpp"
#include "gui/TransformWidget.h"

entity* CameraFactory::createCameraOnSphere(
    const cadm::cadf radius,
    const cadm::vec3 target,
    const std::string &name) const
{
    const auto entity = m_scene.createEntity(name);
    const auto camera = entity->addComponent<ProjectionCameraComponent>();
    camera->setRadius(radius);
    camera->setTarget(target);
    return entity;
}

entity* CameraFactory::createCadCamera(
    const cadm::vec3 &position,
    const cadm::vec3 &target,
    const cadm::vec3 &worldUp,
    const std::string &name) const
{
    const auto entity = m_scene.createEntity(name);
    const auto camera = entity->addComponent<cadCameraComponent>();
    camera->setPosition(position);
    camera->setTarget(target);
    camera->setWorldUp(worldUp);
    return entity;
}