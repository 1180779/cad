//
// Created on 6/19/26.
//

#ifndef CAD_COMMANDS_HPP
#define CAD_COMMANDS_HPP

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <cad_math/vec3.hpp>

#include "Command.hpp"
#include "../EntitySpec.hpp"
#include "../PointRegistry.hpp"
#include "../components/Entity.hpp"
#include "../components/EntitySnapshot.hpp"

class Scene;

/// @brief 
/// Create an entity. First, execute() runs builder (e.g., a GeometryFactory call) to allocate the entity; 
/// its spec is captured, so redo rebuilds it identically
class CreateEntityCommand final : public Command {
public:
    CreateEntityCommand(Scene &scene, std::function<Entity * (Scene&)> builder) : m_scene(scene),
        m_builder(std::move(builder)) {}

    void execute() override;

    void undo() override;

private:
    Scene &m_scene;
    std::function<Entity * (Scene&)> m_builder;

    /// @brief Spec of the created entity; empty until the first execute() captures it
    std::optional<EntitySpec> m_spec;
};

/// @brief 
/// Delete one or more entities, restoring them 
/// (and any curve membership broken by deleting a shared control point) 
/// on undo
class DeleteEntityCommand final : public Command {
public:
    DeleteEntityCommand(Scene &scene, const std::vector<EntityID> &ids);

    void execute() override;

    void undo() override;

private:
    struct CurveMembership {
        EntityID curveId{};
        std::vector<PointHandle> controlPoints;
    };

    Scene &m_scene;
    std::vector<EntitySpec> m_specs;
    std::vector<CurveMembership> m_affectedCurves;
};

/// @brief Restore the before/after transform snapshots produced by a gizmo gesture
class TransformCommand final : public Command {
public:
    TransformCommand(
        Scene &scene,
        std::vector<EntitySnapshot> before,
        std::vector<EntitySnapshot> after
    ) : m_scene(scene), m_before(std::move(before)), m_after(std::move(after)) {}

    void execute() override;

    void undo() override;

private:
    void restore(const std::vector<EntitySnapshot> &snaps) const;

    Scene &m_scene;
    std::vector<EntitySnapshot> m_before;
    std::vector<EntitySnapshot> m_after;
};

/// @brief Move a single point; 
/// consecutive moves of the same point coalesce
class MovePointCommand final : public Command {
public:
    MovePointCommand(
        Scene &scene,
        const PointHandle handle,
        const cadm::vec3 before,
        const cadm::vec3 after
    ) : m_scene(scene), m_handle(handle), m_before(before), m_after(after) {}

    void execute() override;

    void undo() override;

    bool tryMerge(const Command &next) override;

private:
    Scene &m_scene;
    PointHandle m_handle;
    cadm::vec3 m_before;
    cadm::vec3 m_after;
};

/// @brief Rename an entity
class RenameCommand final : public Command {
public:
    RenameCommand(Scene &scene, const EntityID id, std::string before, std::string after) : m_scene(scene),
        m_id(id),
        m_before(std::move(before)),
        m_after(std::move(after)) {}

    void execute() override;

    void undo() override;

private:
    Scene &m_scene;
    EntityID m_id;
    std::string m_before;
    std::string m_after;
};

/// @brief Generic scalar/property edit. 
/// apply`/`revert perform the writing; 
/// mergeKey (typically the source widget pointer) coalesces a continuous edit gesture
class SetPropertyCommand final : public Command {
public:
    SetPropertyCommand(
        std::function<void()> apply,
        std::function<void()> revert,
        const void *mergeKey
    ) : m_apply(std::move(apply)), m_revert(std::move(revert)), m_mergeKey(mergeKey) {}

    void execute() override {
        m_apply();
    }

    void undo() override {
        m_revert();
    }

    bool tryMerge(const Command &next) override;

private:
    std::function<void()> m_apply;
    std::function<void()> m_revert;
    const void *m_mergeKey;
};

/// @brief Add a control point to a curve entity
class AddControlPointCommand final : public Command {
public:
    AddControlPointCommand(Scene &scene, const EntityID curveId, const PointHandle handle) : m_scene(scene),
        m_curveId(curveId),
        m_handle(handle) {}

    void execute() override;

    void undo() override;

private:
    Scene &m_scene;
    EntityID m_curveId;
    PointHandle m_handle;
};

/// @brief Remove a control point from a curve entity
class RemoveControlPointCommand final : public Command {
public:
    RemoveControlPointCommand(Scene &scene, EntityID curveId, PointHandle handle);

    void execute() override;

    void undo() override;

private:
    Scene &m_scene;
    EntityID m_curveId;
    PointHandle m_handle;

    /// @brief Membership snapshot to restore exact order
    std::vector<PointHandle> m_before;
};

#endif //CAD_COMMANDS_HPP
