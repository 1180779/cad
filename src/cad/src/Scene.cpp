//
// Created on 3/15/26.
//

#include "Scene.hpp"

#include <ranges>
#include <algorithm>
#include "components/PointComponent.hpp"

Entity* Scene::createEntity(const std::string &name) {
    EntityId entityId = m_nextEntityId++;
    m_entities.emplace_back(std::make_unique<Entity>(entityId, name));
    return &*m_entities.back();
}

Entity* Scene::createEntityWithId(const EntityId id, const std::string &name) {
    m_entities.emplace_back(std::make_unique<Entity>(id, name));
    if (id >= m_nextEntityId) {
        m_nextEntityId = id + 1;
    }
    return &*m_entities.back();
}

void Scene::attachPointComponent(Entity *entity, const PointHandle handle, const cadm::vec3 position) {
    m_pointRegistry.addPointAt(handle, position);
    entity->addComponent<PointComponent>(handle);
    m_pointEntityMap[handle] = entity->getId();
}

void Scene::setPointPosition(const PointHandle handle, const cadm::vec3 position) {
    m_pointRegistry.setPosition(handle, position);
}

void Scene::setEntityName(const EntityId id, const std::string &name) {
    if (const auto e = getEntity(id)) {
        e.value()->setName(name);
    }
}

std::optional<Entity*> Scene::getEntity(EntityId id) {
    const auto result = std::ranges::find_if(
        m_entities,
        [id](const std::unique_ptr<Entity> &e) {
            return e->getId() == id;
        }
    );
    if (result != m_entities.end()) {
        return result->get();
    }
    return std::nullopt;
}

std::optional<Entity*> Scene::getEntityByName(const std::string &name) {
    const auto byName = std::ranges::find_if(
        m_entities,
        [name](const std::unique_ptr<Entity> &e) {
            return e->getName() == name;
        }
    );
    if (byName != m_entities.end()) {
        return byName->get();
    }
    return std::nullopt;
}

Entity* Scene::createPoint(const cadm::vec3 position, const std::string &name) {
    Entity *entity = createEntity(name);
    const PointHandle handle = m_pointRegistry.addPoint(position);
    entity->addComponent<PointComponent>(handle);
    m_pointEntityMap[handle] = entity->getId();
    return entity;
}

std::optional<Entity*> Scene::getEntityByPointHandle(const PointHandle handle) {
    const auto it = m_pointEntityMap.find(handle);
    if (it == m_pointEntityMap.end()) {
        return std::nullopt;
    }
    return getEntity(it->second);
}

void Scene::syncPointSelectionToRegistry() {
    for (const auto &e : m_entities) {
        if (const auto pc = e->getComponent<PointComponent>()) {
            m_pointRegistry.setSelected(pc.value()->m_handle, e->isSelected());
        }
    }
}

bool Scene::removeEntity(EntityId id) {
    // pop and replace
    const auto toBeRemoved = std::ranges::find_if(
        m_entities,
        [id](const std::unique_ptr<Entity> &e) {
            return e->getId() == id;
        }
    );
    if (toBeRemoved == m_entities.end()) {
        return false;
    }
    if (m_activeCursor == toBeRemoved->get()) {
        m_activeCursor = nullptr;
    }
    if (m_newPointsTargetEntity == toBeRemoved->get()) {
        m_newPointsTargetEntity = nullptr;
    }
    m_selectedEntities.erase(toBeRemoved->get());
    if (const auto pc = (*toBeRemoved)->getComponent<PointComponent>()) {
        m_pointEntityMap.erase(pc.value()->m_handle);
        m_pointRegistry.removePoint(pc.value()->m_handle);
    }
    toBeRemoved->swap(m_entities.back());
    m_entities.pop_back();
    return true;
}

auto Scene::getVisibleEntities() {
    return m_entities | std::views::filter(
        [](const std::unique_ptr<Entity> &e) {
            return e->isVisible();
        }
    );
}

void Scene::setSelected(Entity *e, const bool selected) {
    e->setSelected(selected, SelectionKey{});
    if (selected) {
        m_selectedEntities.insert(e);
    }
    else {
        m_selectedEntities.erase(e);
    }
}

void Scene::clearSelection() {
    for (auto *e : m_selectedEntities) {
        e->setSelected(false, SelectionKey{});
    }
    m_selectedEntities.clear();
}
