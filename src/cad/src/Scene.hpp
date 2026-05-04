//
// Created on 3/15/26.
//

#ifndef CAD_SCENE_H
#define CAD_SCENE_H

#include "components/Entity.hpp"
#include "PointRegistry.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <ranges>
#include <unordered_map>

class Scene
{
public:
    Entity* createEntity(const std::string &name = "Entity");
    Entity* createPoint(cadm::vec3 position, const std::string &name = "Point");
    std::optional<Entity*> getEntity(EntityID id);
    std::optional<Entity*> getEntityByName(const std::string &name);
    std::optional<Entity*> getEntityByPointHandle(PointHandle handle);
    void removeEntity(EntityID id);

    void syncPointSelectionToRegistry();

    const std::vector<std::unique_ptr<Entity>>& getEntities() const { return m_entities; }
    auto getVisibleEntities();
    auto getSelectedEntities();

    Entity* getActiveCursor() const { return m_activeCursor; }
    void setActiveCursor(Entity *cursor) { m_activeCursor = cursor; }

    Entity* getActiveBezierC0() const { return m_activeBezierC0; }
    void setActiveBezierC0(Entity *e) { m_activeBezierC0 = e; }

    PointRegistry& getPointRegistry() { return m_pointRegistry; }
    const PointRegistry& getPointRegistry() const { return m_pointRegistry; }

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unordered_map<EntityID, std::size_t> m_entityMap;
    std::unordered_map<PointHandle, EntityID> m_pointEntityMap;
    EntityID m_nextEntityId = 1;
    Entity *m_activeCursor = nullptr;
    Entity *m_activeBezierC0 = nullptr;
    PointRegistry m_pointRegistry;
};

#endif //CAD_SCENE_H
