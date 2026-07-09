//
// Created on 6/19/26.
//

#include <algorithm>

#include "EntitySpec.hpp"

#include "Scene.hpp"
#include "Tools.hxx"
#include "components/geometry/BezierC0Component.hpp"
#include "components/geometry/BezierC2Component.hpp"
#include "components/geometry/GregoryComponent.hxx"
#include "components/geometry/InterpC2Component.hxx"
#include "components/CursorComponent.hpp"
#include "components/GeometryComponent.hpp"
#include "components/geometry/PatchC0Component.hxx"
#include "components/geometry/PatchC2Component.hxx"
#include "components/PointComponent.hpp"
#include "components/TransformComponent.hpp"

namespace {
    void setPatch(PatchComponent*patch, const PatchGridData*d);
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
    if (const auto ic = entity->getComponent<InterpC2Component>()) {
        out.components.emplace_back(
            InterpC2Data{
                ic.value()->getControlPoints(),
                ic.value()->getShowControlPolyline(),
                ic.value()->getShowBernsteinPolygon(),
                ic.value()->getShowBernsteinCps()
            }
        );
    }
    if (const auto gc = entity->getComponent<GregoryComponent>()) {
        out.components.emplace_back(
            GregoryData{
                gc.value()->controlPointHandles(),
                gc.value()->gridDivisionsU(),
                gc.value()->gridDivisionsV(),
                gc.value()->getShowVectors()
            }
        );
    }
    if (const auto pc = entity->getComponent<PatchComponent>()) {
        const auto *p = pc.value();
        const PatchGridData grid{
            p->getControlPoints(),
            p->getRows(),
            p->getCols(),
            p->getWrapU(),
            p->getPatchCountX(),
            p->getPatchCountY(),
            p->getGridDivisionsU(),
            p->getGridDivisionsV(),
            p->getShowNet()
        };
        if (dynamic_cast<const PatchC2Component*>(p)) {
            out.components.emplace_back(PatchC2Data{grid});
        }
        else if (dynamic_cast<const PatchC0Component*>(p)) {
            out.components.emplace_back(PatchC0Data{grid});
        }
    }

    return !out.components.empty();
}

Entity* rebuildEntity(Scene &scene, const EntitySpec &spec) {
    Entity *e = scene.createEntityWithId(spec.id, spec.name);

    for (const auto &component : spec.components) {
        std::visit(
            tools::Overloaded{
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
                [&](const InterpC2Data &d) {
                    const auto curve = e->addComponent<InterpC2Component>(&scene.getPointRegistry());
                    for (const auto h : d.controlPoints) {
                        curve->addControlPoint(h);
                    }
                    curve->setShowControlPolyline(d.showPolyline);
                    curve->setShowBernsteinPolygon(d.showBernsteinPolygon);
                    curve->setShowBernsteinCps(d.showBernsteinCps);
                },
                [&](const PatchC0Data &d) {
                    auto *patch = e->addComponent<PatchC0Component>(&scene.getPointRegistry());
                    setPatch(patch, &d);
                },
                [&](const PatchC2Data &d) {
                    auto *patch = e->addComponent<PatchC2Component>(&scene.getPointRegistry());
                    setPatch(patch, &d);
                },
                [&](const GregoryData &d) {
                    auto *gregory = e->addComponent<GregoryComponent>(&scene.getPointRegistry());
                    gregory->setHole(d.controlPoints);
                    const int nets = std::min(
                        gregory->netCount(),
                        static_cast<int>(std::min(d.gridDivisionsU.size(), d.gridDivisionsV.size()))
                    );
                    for (int net = 0; net < nets; ++net) {
                        gregory->setGridDivisionsU(net, d.gridDivisionsU[net]);
                        gregory->setGridDivisionsV(net, d.gridDivisionsV[net]);
                    }
                    gregory->setShowVectors(d.showVectors);
                },
            },
            component
        );
    }

    e->setVisible(spec.visible);
    return e;
}

namespace {
    void setPatch(PatchComponent* const patch, const PatchGridData* const d) {
        patch->setGrid(d->controlPoints, d->rows, d->cols, d->wrapU, d->patchCountX, d->patchCountY);
        patch->setGridDivisionsU(d->gridDivisionsU);
        patch->setGridDivisionsV(d->gridDivisionsV);
        patch->setShowNet(d->showNet);
    }
}
