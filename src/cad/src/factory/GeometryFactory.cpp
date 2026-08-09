//
// Created on 3/15/26.
//

#include "GeometryFactory.hpp"

#include "../PatchGeometry.hxx"
#include "../components/geometry/BezierC0Component.hpp"
#include "../components/geometry/BezierC2Component.hpp"
#include "../components/geometry/GregoryComponent.hxx"
#include "../components/geometry/IntersectionCurveComponent.hxx"
#include "../components/geometry/InterpC2Component.hxx"
#include "../components/CursorComponent.hpp"
#include "../components/GeometryComponent.hpp"
#include "../components/geometry/PatchC0Component.hxx"
#include "../components/geometry/PatchC2Component.hxx"
#include "../utils/IntersectionUtils.hxx"
#include "../components/PointComponent.hpp"
#include "../components/TransformComponent.hpp"

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
    const auto torus = entity->addComponent<TorusComponent>();
    torus->setMajorRadius(majorRadius);
    torus->setMinorRadius(minorRadius);
    torus->setMajorSegments(majorSegments);
    torus->setMinorSegments(minorSegments);
    return entity;
}

Entity* GeometryFactory::createAxis(const float length, const cadm::Vec3 &position, const std::string &name) const {
    const auto e = m_scene.createEntity(name);
    e->addComponent<TransformComponent>()->setTranslation(position);
    auto *axes = e->addComponent<AxesComponent>();
    axes->m_length = length;
    return e;
}

Entity* GeometryFactory::createCursor(const cadm::Vec3 &position, const std::string &name) const {
    const auto e = m_scene.createEntity(name);
    e->addComponent<TransformComponent>()->setTranslation(position);
    auto *axes = e->addComponent<AxesComponent>();
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

Entity* GeometryFactory::createGregory(
    const std::vector<PointHandle> &holeHandles,
    const std::string &name
) const {
    const auto entity = m_scene.createEntity(name);
    auto *gregory = entity->addComponent<GregoryComponent>(&m_scene.getPointRegistry());
    gregory->setHole(holeHandles);
    return entity;
}

Entity* GeometryFactory::createIntersectionCurve(
    const EntityId patch1,
    const EntityId patch2,
    const intersections::IntersectionCurve &curve,
    const intersections::IntersectionCurveData &data,
    const trimming::SurfaceWrap wrap1,
    const trimming::SurfaceWrap wrap2,
    const std::string &name
) const {
    const auto entity = m_scene.createEntity(name);
    entity->addComponent<TransformComponent>();
    entity->addComponent<IntersectionCurveComponent>(
        patch1,
        patch2,
        data.points3D,
        data.params1,
        data.params2,
        curve.closed,
        wrap1,
        wrap2
    );
    return entity;
}

std::vector<Entity*> GeometryFactory::createPatch(const patchgen::PatchCreateParams &params) const {
    const auto [rows, cols, wrap, patchCountX, patchCountY, positions] = patchgen::generate(params);

    std::vector<Entity*> created;
    created.reserve(positions.size() + 1);
    std::vector<PointHandle> handles;
    handles.reserve(positions.size());

    for (const auto &pos : positions) {
        Entity *pe = m_scene.createPoint(pos, "Patch Point");
        created.push_back(pe);
        handles.push_back(pe->getComponent<PointComponent>().value()->m_handle);
    }

    const auto entity = m_scene.createEntity(
        params.type == patchgen::PatchCreateParams::Type::c2
            ? "Patch C2"
            : "Patch C0"
    );
    PatchComponent *patch = params.type == patchgen::PatchCreateParams::Type::c2
                                ? static_cast<PatchComponent*>(
                                    entity->addComponent<PatchC2Component>(&m_scene.getPointRegistry())
                                )
                                : entity->addComponent<PatchC0Component>(&m_scene.getPointRegistry());
    patch->setGrid(std::move(handles), rows, cols, wrap, patchCountX, patchCountY);
    created.push_back(entity);
    return created;
}

std::vector<Entity*> GeometryFactory::createInterpolatedFromPoints(
    const std::vector<cadm::Vec3> &points,
    const int everyNth,
    const std::string &name
) const {
    std::vector<Entity*> created;
    if (points.empty()) {
        return created;
    }
    const auto stride = static_cast<std::size_t>(std::max(1, everyNth));

    std::vector<PointHandle> handles;
    handles.reserve(points.size() / stride + 1);
    for (std::size_t i = 0; i < points.size(); i += stride) {
        Entity *pe = m_scene.createPoint(points[i], "Spline Point");
        created.push_back(pe);
        handles.push_back(pe->getComponent<PointComponent>().value()->m_handle);
    }
    if ((points.size() - 1) % stride != 0) {
        Entity *pe = m_scene.createPoint(points.back(), "Spline Point");
        created.push_back(pe);
        handles.push_back(pe->getComponent<PointComponent>().value()->m_handle);
    }

    const auto entity = m_scene.createEntity(name);
    auto *curve = entity->addComponent<InterpC2Component>(&m_scene.getPointRegistry());
    for (const auto h : handles) {
        curve->addControlPoint(h);
    }
    created.push_back(entity);
    return created;
}
