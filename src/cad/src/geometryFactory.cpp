//
// Created on 3/15/26.
//

#include "geometryFactory.h"

#include "components/geometry.h"
#include "components/transform.h"

entity* GeometryFactory::createTorus(
    const float majorRadius,
    const float minorRadius,
    const int majorSegments,
    const int minorSegments,
    const cadm::vec3 &position,
    const std::string &name) const
{
    const auto entity = m_scene.createEntity(name);
    const auto transform = entity->addComponent<TransformComponent>();
    transform->setTranslation(position);
    const auto torus = entity->addComponent<TorusGeometry>();
    torus->setMajorRadius(majorRadius);
    torus->setMinorRadius(minorRadius);
    torus->setMajorSegments(majorSegments);
    torus->setMinorSegments(minorSegments);
    return entity;
}

entity* GeometryFactory::createAxis(float length, const cadm::vec3 &position, const std::string &name) const
{
    const auto entity = m_scene.createEntity();
    // TODO: implement
    return entity;
}

entity* GeometryFactory::createGrid(
    float size,
    int divisions,
    const cadm::vec3 &position,
    const std::string &name) const
{
    const auto entity = m_scene.createEntity();
    // TODO: implement
    return entity;
}
