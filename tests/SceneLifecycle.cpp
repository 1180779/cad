#include <catch2/catch_test_macros.hpp>

#include "CameraFactory.hpp"
#include "GeometryFactory.hpp"
#include "PersistentEntities.hxx"
#include "Scene.hpp"
#include "components/PatchC0Component.hxx"
#include "components/PointComponent.hpp"

namespace {
    PointHandle handleOf(Entity *e) {
        return e->getComponent<PointComponent>().value()->m_handle;
    }
}

TEST_CASE("removeEntity cleans up every geometry component type", "[scene]") {
    Scene scene;
    const GeometryFactory factory(scene);

    SECTION("point") {
        Entity *p = factory.createPoint({1, 2, 3});
        REQUIRE(scene.removeEntity(p->getId()));
        REQUIRE(scene.getEntities().empty());
    }

    SECTION("torus") {
        Entity *t = factory.createTorus(2.0f, 0.5f, 8, 8);
        REQUIRE(scene.removeEntity(t->getId()));
        REQUIRE(scene.getEntities().empty());
    }

    SECTION("cursor (axes)") {
        Entity *c = factory.createCursor();
        REQUIRE(scene.removeEntity(c->getId()));
        REQUIRE(scene.getEntities().empty());
    }

    SECTION("bezierC0") {
        Entity *p = factory.createPoint();
        Entity *b = factory.createBezierC0({handleOf(p)});
        REQUIRE(scene.removeEntity(b->getId()));
        REQUIRE(scene.removeEntity(p->getId()));
        REQUIRE(scene.getEntities().empty());
    }

    SECTION("bezierC2") {
        Entity *p = factory.createPoint();
        Entity *b = factory.createBezierC2({handleOf(p)});
        REQUIRE(scene.removeEntity(b->getId()));
        REQUIRE(scene.removeEntity(p->getId()));
        REQUIRE(scene.getEntities().empty());
    }

    SECTION("interpolatedC2") {
        Entity *p = factory.createPoint();
        Entity *ic = factory.createInterpC2({handleOf(p)});
        REQUIRE(scene.removeEntity(ic->getId()));
        REQUIRE(scene.removeEntity(p->getId()));
        REQUIRE(scene.getEntities().empty());
    }

    SECTION("patch") {
        std::vector<PointHandle> handles;
        std::vector<Entity*> points;
        for (int i = 0; i < 4; ++i) {
            Entity *p = factory.createPoint({static_cast<cadm::cadf>(i), 0, 0});
            points.push_back(p);
            handles.push_back(handleOf(p));
        }
        Entity *patchEntity = scene.createEntity("Patch");
        auto *patch = patchEntity->addComponent<PatchC0Component>(&scene.getPointRegistry());
        patch->setGrid(handles, 2, 2, false, 1, 1);

        // a control point locked by a still-attached patch refuses to be removed
        REQUIRE_FALSE(scene.removeEntity(points[0]->getId()));

        // removing the patch unlocks its control points
        REQUIRE(scene.removeEntity(patchEntity->getId()));
        for (Entity *p : points) {
            REQUIRE(scene.removeEntity(p->getId()));
        }
        REQUIRE(scene.getEntities().empty());
    }
}

TEST_CASE("Scene::reset is order-independent", "[scene]") {
    Scene scene;
    const GeometryFactory factory(scene);

    std::vector<PointHandle> handles;
    for (int i = 0; i < 4; ++i) {
        handles.push_back(handleOf(factory.createPoint({static_cast<cadm::cadf>(i), 0, 0})));
    }
    Entity *patchEntity = scene.createEntity("Patch");
    auto *patch = patchEntity->addComponent<PatchC0Component>(&scene.getPointRegistry());
    patch->setGrid(handles, 2, 2, false, 1, 1);

    REQUIRE(scene.tryReset());
    REQUIRE(scene.getEntities().empty());
    REQUIRE(scene.createEntity()->getId() == 1);
}

TEST_CASE("PersistentEntities detaches and reattaches entities with fresh ids", "[scene]") {
    Scene scene;
    const GeometryFactory geometryFactory(scene);
    const CameraFactory cameraFactory(scene);

    Entity *cursor = geometryFactory.createCursor();
    scene.setActiveCursor(cursor);
    Entity *blenderCam = cameraFactory.createBlenderCamera(20, {});
    Entity *cadCam = cameraFactory.createCadCamera({0, 0, -10}, {}, cadm::Vec3::unitY());

    PersistentEntities persistent;
    persistent.detachFrom(scene);
    REQUIRE(scene.getEntities().empty());

    REQUIRE(scene.tryReset());
    Entity *loadedPoint = geometryFactory.createPoint();

    persistent.reattachTo(scene);

    REQUIRE(scene.getEntities().size() == 4);
    REQUIRE(scene.getActiveCursor() == cursor);
    REQUIRE(cursor->getId() != loadedPoint->getId());
    REQUIRE(blenderCam->getId() != loadedPoint->getId());
    REQUIRE(cadCam->getId() != loadedPoint->getId());
}
