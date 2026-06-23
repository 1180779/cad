//
// Created on 3/15/26.
//

#include "GeometryFactory.hpp"

#include "components/BezierC0Component.hpp"
#include "components/BezierC2Component.hpp"
#include "components/InterpC2Component.hxx"
#include "components/CursorComponent.hpp"
#include "components/GeometryComponent.hpp"
#include "components/TransformComponent.hpp"

Entity* GeometryFactory::createTorus(
    const float majorRadius,
    const float minorRadius,
    const int majorSegments,
    const int minorSegments,
    const cadm::Vec3 &position,
    const std::string &name
) const {
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

Entity* GeometryFactory::createAxis(const float length, const cadm::Vec3 &position, const std::string &name) const {
    const auto e = m_scene.createEntity(name);
    e->addComponent<TransformComponent>()->setTranslation(position);
    auto *axes = e->addComponent<AxesGeometry>();
    axes->m_length = length;
    return e;
}

Entity* GeometryFactory::createCursor(const cadm::Vec3 &position, const std::string &name) const {
    const auto e = m_scene.createEntity(name);
    e->addComponent<TransformComponent>()->setTranslation(position);
    auto *axes = e->addComponent<AxesGeometry>();
    axes->m_length = 0.5f;
    e->addComponent<CursorComponent>();
    return e;
}

Entity* GeometryFactory::createPoint(const cadm::Vec3 &position, const std::string &name) const {
    return m_scene.createPoint(position, name);
}

Entity* GeometryFactory::createBezierC0(
    const std::vector<PointHandle> &controlPoints,
    const std::string &name
) const {
    const auto entity = m_scene.createEntity(name);
    auto *bezier = entity->addComponent<BezierC0Component>(&m_scene.getPointRegistry());
    for (const auto h : controlPoints) {
        bezier->addControlPoint(h);
    }
    return entity;
}

Entity* GeometryFactory::createBezierC2(
    const std::vector<PointHandle> &controlPoints,
    const std::string &name
) const {
    const auto entity = m_scene.createEntity(name);
    auto *bezier = entity->addComponent<BezierC2Component>(&m_scene.getPointRegistry());
    for (const auto h : controlPoints) {
        bezier->addControlPoint(h);
    }
    return entity;
}

Entity* GeometryFactory::createInterpC2(
    const std::vector<PointHandle> &controlPoints,
    const std::string &name
) const {
    const auto entity = m_scene.createEntity(name);
    auto *curve = entity->addComponent<InterpC2Component>(&m_scene.getPointRegistry());
    for (const auto h : controlPoints) {
        curve->addControlPoint(h);
    }
    return entity;
}
