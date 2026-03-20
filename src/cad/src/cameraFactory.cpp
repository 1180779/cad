//
// Created on 3/17/26.
//

#include "cameraFactory.hpp"

#include "components/camera.hpp"
#include "gui/TransformWidget.h"

entity* CameraFactory::createArcBallCamera(
    const cadm::cadf radius,
    const cadm::vec3 target,
    const cadm::vec3 worldUp,
    const std::string &name) const
{
    const auto entity = m_scene.createEntity(name);
    const auto camera = entity->addComponent<CameraComponent>();
    camera->setRadius(radius);
    camera->setTarget(target);
    return entity;
}