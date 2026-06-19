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
#include <unordered_set>

class Scene {
public:
    Entity* createEntity(const std::string &name = "Entity");

    /// @brief Create an entity with a specific id 
    /// (used to resurrect a deleted entity during undo so existing references stay valid). 
    /// Keeps m_nextEntityId ahead
    Entity* createEntityWithId(EntityId id, const std::string &name = "Entity");

    Entity* createPoint(cadm::Vec3 position, const std::string &name = "Point");

    /// @brief Attach a PointComponent to an existing entity, registering the handle in the
    /// point registry at that exact slot. Used when rebuilding a serialized entity
    void attachPointComponent(Entity *entity, PointHandle handle, cadm::Vec3 position);

    /// @brief Convenience mutator used by commands to write a point position
    /// @note: not an exclusive surface
    void setPointPosition(PointHandle handle, cadm::Vec3 position);

    void setEntityName(EntityId id, const std::string &name);

    std::optional<Entity*> getEntity(EntityId id);

    std::optional<Entity*> getEntityByName(const std::string &name);

    std::optional<Entity*> getEntityByPointHandle(PointHandle handle);

    bool removeEntity(EntityId id);

    /// @brief Set the selection state on an entity and keep the selection set in sync
    void setSelected(Entity *e, bool selected);

    /// @brief Deselect all entities and clear the selection set
    void clearSelection();

    void syncPointSelectionToRegistry();

    const std::vector<std::unique_ptr<Entity>>& getEntities() const {
        return m_entities;
    }

    [[nodiscard]] const std::unordered_set<Entity*>& getSelectedEntities() const {
        return m_selectedEntities;
    }

    auto getVisibleEntities();

    Entity* getActiveCursor() const {
        return m_activeCursor;
    }

    void setActiveCursor(Entity *cursor) {
        m_activeCursor = cursor;
    }

    Entity* getNewPointsTargetEntity() const {
        return m_newPointsTargetEntity;
    }

    void setNewPointsTargetEntity(Entity *e) {
        m_newPointsTargetEntity = e;
    }

    PointRegistry& getPointRegistry() {
        return m_pointRegistry;
    }

    const PointRegistry& getPointRegistry() const {
        return m_pointRegistry;
    }

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unordered_map<EntityId, std::size_t> m_entityMap;
    std::unordered_map<PointHandle, EntityId> m_pointEntityMap;
    EntityId m_nextEntityId = 1;
    Entity *m_activeCursor = nullptr;

    Entity *m_newPointsTargetEntity = nullptr;
    PointRegistry m_pointRegistry;
    std::unordered_set<Entity*> m_selectedEntities;
};

#endif //CAD_SCENE_H
