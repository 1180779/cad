//
// Created on 3/17/26.
//

#include "cameraFactory.hpp"

#include "components/camera.hpp"
#include "gui/TransformWidget.h"

entity* CameraFactory::createArcBallCamera(
    const cadm::vec3 positon,
    const cadm::vec3 target,
    const cadm::vec3 worldUp) const
{
    const auto entity = m_scene.createEntity();
    const auto camera = entity->addComponent<CameraComponent>();
    camera->m_position = positon;
    camera->m_target = target;
    camera->m_worldUp = worldUp;
    return entity;
}