//
// Created on 6/19/26.
//

#include "EntitySpec.hpp"

#include "Scene.hpp"
#include "components/BezierC0Component.hpp"
#include "components/BezierC2Component.hpp"
#include "components/CursorComponent.hpp"
#include "components/GeometryComponent.hpp"
#include "components/PointComponent.hpp"
#include "components/TransformComponent.hpp"

namespace {
    /// @brief Helper for exhaustive std::visit
    template <typename... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };
}

bool captureEntity(Scene &scene, Entity *entity, EntitySpec &out) {
    out.id = entity->getId();
    out.name = entity->getName();
    out.visible = entity->isVisible();
    out.components.clear();

    // probe each known component type and append its payload
    // transform first so it exists before geometry on rebuild
    if (const auto t = entity->getComponent<TransformComponent>()) {
        out.components.emplace_back(
            TransformData{
                t.value()->getTranslation(),
                t.value()->getRotation(),
                t.value()->getScale()
            }
        );
    }
    if (const auto pc = entity->getComponent<PointComponent>()) {
        const PointHandle h = pc.value()->m_handle;
        out.components.emplace_back(PointData{h, scene.getPointRegistry().getPosition(h)});
    }
    if (const auto tg = entity->getComponent<TorusGeometry>()) {
        out.components.emplace_back(
            TorusData{
                tg.value()->getMajorRadius(),
                tg.value()->getMinorRadius(),
                tg.value()->getMajorSegments(),
                tg.value()->getMinorSegments()
            }
        );
    }
    if (const auto axes = entity->getComponent<AxesGeometry>()) {
        out.components.emplace_back(AxesData{axes.value()->m_length});
    }
    if (entity->hasComponent<CursorComponent>()) {
        out.components.emplace_back(CursorData{});
    }
    if (const auto bc = entity->getComponent<BezierC0Component>()) {
        out.components.emplace_back(BezierC0Data{bc.value()->getControlPoints(), bc.value()->getShowPolygon()});
    }
    if (const auto bc = entity->getComponent<BezierC2Component>()) {
        out.components.emplace_back(
            BezierC2Data{
                bc.value()->getDeBoorPoints(),
                bc.value()->getShowDeBoorPolygon(),
                bc.value()->getShowBernsteinPolygon()
            }
        );
    }

    return !out.components.empty();
}

Entity* rebuildEntity(Scene &scene, const EntitySpec &spec) {
    Entity *e = scene.createEntityWithId(spec.id, spec.name);

    for (const auto &component : spec.components) {
        std::visit(
            Overloaded{
                [&](const TransformData &d) {
                    const auto t = e->addComponent<TransformComponent>();
                    t->setTranslation(d.translation);
                    t->setRotation(d.rotation);
                    t->setScale(d.scale);
                },
                [&](const PointData &d) {
                    scene.attachPointComponent(e, d.handle, d.position);
                },
                [&](const TorusData &d) {
                    const auto torus = e->addComponent<TorusGeometry>();
                    torus->setMajorRadius(d.majorRadius);
                    torus->setMinorRadius(d.minorRadius);
                    torus->setMajorSegments(d.majorSegments);
                    torus->setMinorSegments(d.minorSegments);
                },
                [&](const AxesData &d) {
                    e->addComponent<AxesGeometry>()->m_length = d.length;
                },
                [&](const CursorData &) {
                    e->addComponent<CursorComponent>();
                },
                [&](const BezierC0Data &d) {
                    const auto bezier = e->addComponent<BezierC0Component>(&scene.getPointRegistry());
                    for (const auto h : d.controlPoints) {
                        bezier->addControlPoint(h);
                    }
                    bezier->setShowPolygon(d.showPolygon);
                },
                [&](const BezierC2Data &d) {
                    const auto bezier = e->addComponent<BezierC2Component>(&scene.getPointRegistry());
                    for (const auto h : d.controlPoints) {
                        bezier->addControlPoint(h);
                    }
                    bezier->setShowDeBoorPolygon(d.showDeBoorPolygon);
                    bezier->setShowBernsteinPolygon(d.showBernsteinPolygon);
                    bezier->setShowBernsteinCps(d.showBernsteinCps);
                },
            },
            component
        );
    }

    e->setVisible(spec.visible);
    return e;
}
