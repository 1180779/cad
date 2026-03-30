//
// Created on 3/15/26.
//

#ifndef CAD_SCENE_H
#define CAD_SCENE_H

#include "entities/entity.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <ranges>
#include <unordered_map>

class Scene
{
public:
    entity* createEntity(const std::string &name = "Entity");
    std::optional<entity*> getEntity(EntityID id);
    std::optional<entity*> getEntityByName(const std::string &name);
    void removeEntity(EntityID id);

    const std::vector<std::unique_ptr<entity>>& getEntities() const { return m_entities; }
    auto getVisibleEntities();
    auto getSelectedEntities();

    entity* getActiveCursor() const { return m_activeCursor; }
    void setActiveCursor(entity *cursor) { m_activeCursor = cursor; }

private:
    std::vector<std::unique_ptr<entity>> m_entities;
    std::unordered_map<EntityID, std::size_t> m_entityMap;
    EntityID m_nextEntityId = 1;
    entity *m_activeCursor = nullptr;
};

#endif //CAD_SCENE_H