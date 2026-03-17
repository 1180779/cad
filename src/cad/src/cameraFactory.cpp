//
// Created on 3/17/26.
//

#include "cameraFactory.hpp"

#include "components/camera.hpp"
#include "gui/TransformWidget.h"

entity* CameraFactory::createLookAtCamera(const cadm::vec3 positon, cadm::vec3 target, cadm::vec3 up) const
{
    const auto entity = m_scene.createEntity();
    const auto transform = entity->addComponent<TransformComponent>();
    transform->setTranslation(positon);
    // TODO: use target to set rotation
    entity->addComponent<CameraComponent>(up);
    return entity;
}
