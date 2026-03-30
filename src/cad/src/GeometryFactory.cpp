//
// Created on 3/15/26.
//

#include "GeometryFactory.hpp"

#include "components/CursorComponent.hpp"
#include "components/GeometryComponent.hpp"
#include "components/TransformComponent.hpp"

Entity* GeometryFactory::createTorus(
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

Entity* GeometryFactory::createAxis(const float length, const cadm::vec3 &position, const std::string &name) const
{
    const auto e = m_scene.createEntity(name);
    e->addComponent<TransformComponent>()->setTranslation(position);
    auto *axes = e->addComponent<AxesGeometry>();
    axes->m_length = length;
    return e;
}

Entity* GeometryFactory::createCursor(const cadm::vec3 &position, const std::string &name) const
{
    const auto e = m_scene.createEntity(name);
    e->addComponent<TransformComponent>()->setTranslation(position);
    auto *axes = e->addComponent<AxesGeometry>();
    axes->m_length = 0.5f;
    e->addComponent<CursorComponent>();
    return e;
}
