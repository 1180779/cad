//
// Created on 3/15/26.
//

#ifndef CAD_SCENE_H
#define CAD_SCENE_H

#include "entities/Entity.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <ranges>
#include <unordered_map>

class Scene
{
public:
    Entity* createEntity(const std::string &name = "Entity");
    std::optional<Entity*> getEntity(EntityID id);
    std::optional<Entity*> getEntityByName(const std::string &name);
    void removeEntity(EntityID id);

    const std::vector<std::unique_ptr<Entity>>& getEntities() const { return m_entities; }
    auto getVisibleEntities();
    auto getSelectedEntities();

    Entity* getActiveCursor() const { return m_activeCursor; }
    void setActiveCursor(Entity *cursor) { m_activeCursor = cursor; }

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unordered_map<EntityID, std::size_t> m_entityMap;
    EntityID m_nextEntityId = 1;
    Entity *m_activeCursor = nullptr;
};

#endif //CAD_SCENE_H