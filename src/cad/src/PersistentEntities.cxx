//
// Created by Radosław Głasek on 07.07.2026
//

#include "PersistentEntities.hxx"

#include "components/camera/BlenderCameraComponent.hpp"
#include "components/camera/CadCameraComponent.hpp"
#include "components/CursorComponent.hpp"
#include "Scene.hpp"

bool isPersistentEntity(const Entity *e) {
    return e->hasComponent<CursorComponent>()
        || e->hasComponent<BlenderCameraComponent>()
        || e->hasComponent<CadCameraComponent>();
}

void PersistentEntities::detachFrom(Scene &scene) {
    std::vector<EntityId> ids;
    for (const auto &e : scene.getEntities()) {
        if (isPersistentEntity(e.get())) {
            ids.push_back(e->getId());
        }
    }
    for (const auto id : ids) {
        m_entities.push_back(scene.releaseEntity(id));
    }
}

void PersistentEntities::reattachTo(Scene &scene) {
    for (auto &e : m_entities) {
        const bool wasCursor = e->hasComponent<CursorComponent>();
        Entity *adopted = scene.adoptEntity(std::move(e));
        if (wasCursor) {
            scene.setActiveCursor(adopted);
        }
    }
    m_entities.clear();
}
