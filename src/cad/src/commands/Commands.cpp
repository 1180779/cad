//
// Created on 6/19/26.
//

#include "Commands.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "../factory/GeometryFactory.hpp"
#include "../Scene.hpp"
#include "../components/geometry/BezierC0Component.hpp"
#include "../components/geometry/BezierC2Component.hpp"
#include "../components/geometry/InterpC2Component.hxx"
#include "../components/INewPointsTargetComponent.hpp"
#include "../components/IPointReferrer.hpp"
#include "../components/geometry/PatchComponent.hxx"
#include "../components/PointComponent.hpp"

namespace {
    /// @brief Read a curve entity's current control point list
    /// @param e The entity to read control points from
    /// @return Vector of PointHandles representing the curve's control points, or empty vector if not a curve
    std::vector<PointHandle> currentControlPoints(Entity *e) {
        if (const auto bc = e->getComponent<BezierC0Component>()) {
            return bc.value()->getControlPoints();
        }
        if (const auto bc = e->getComponent<BezierC2Component>()) {
            return bc.value()->getDeBoorPoints();
        }
        if (const auto ic = e->getComponent<InterpC2Component>()) {
            return ic.value()->getControlPoints();
        }
        return {};
    }

    /// @brief Add every control-point entity of a deleted patch to the deletion set
    /// @return ids, extended in place with the patch's control points
    std::vector<EntityId> expandWithPatchControlPoints(Scene &scene, const std::vector<EntityId> &ids);

    /// @brief Map every point handle owned by a patch to that patch's entity id
    std::unordered_map<PointHandle, EntityId> buildPointToPatchMap(const Scene &scene);

    /// @brief Drop points that are locked to a patch not also being deleted
    void removeLockedPatchPoints(Scene &scene, std::vector<EntityId> &ids);

    /// @brief Capture specs for every deleted entity, and collect the point handles among them
    /// @return {specs, deletedPointHandles}
    std::pair<std::vector<EntitySpec>, std::unordered_set<PointHandle>> captureDeleted(
        Scene &scene,
        const std::vector<EntityId> &ids
    );

    /// @brief The ids of every captured entity spec
    std::vector<EntityId> specIds(const std::vector<EntitySpec> &specs) {
        return specs | std::views::transform(
            [](const auto &s) {
                return s.id;
            }
        ) | std::ranges::to<std::vector>();
    }

    /// @brief Reset a curve's membership to exactly target, preserving order
    void resetMembership(Scene &scene, const EntityId curveId, const std::vector<PointHandle> &target) {
        const auto entity = scene.getEntity(curveId);
        if (!entity) {
            return;
        }
        const auto base = entity.value()->getComponent<INewPointsTargetBase>();
        if (!base) {
            return;
        }
        for (const auto h : currentControlPoints(entity.value())) {
            base.value()->removeControlPoint(h);
        }
        for (const auto h : target) {
            base.value()->addControlPoint(h);
        }
    }
}

// ---------------------------------------------------------------------------
// CreateEntityCommand
// ---------------------------------------------------------------------------

void CreateEntityCommand::execute() {
    if (m_spec) {
        rebuildEntity(m_scene, *m_spec);
        return;
    }
    if (Entity *e = m_builder(m_scene)) {
        if (EntitySpec spec;
            captureEntity(m_scene, e, spec)) {
            m_spec = std::move(spec);
        }
    }
}

void CreateEntityCommand::undo() {
    if (m_spec) {
        m_scene.removeEntity(m_spec->id);
    }
}

// ---------------------------------------------------------------------------
// CreatePatchCommand
// ---------------------------------------------------------------------------

void CreatePatchCommand::execute() {
    if (!m_specs.empty()) {
        // redo: points first so the patch can re-reference live handles
        const auto isPatch = [](const EntitySpec &s) {
            return s.has<PatchC0Data>() || s.has<PatchC2Data>();
        };
        for (const auto &s : m_specs) {
            if (!isPatch(s)) {
                rebuildEntity(m_scene, s);
            }
        }
        for (const auto &s : m_specs) {
            if (isPatch(s)) {
                rebuildEntity(m_scene, s);
            }
        }
        return;
    }

    for (const auto created = GeometryFactory(m_scene).createPatch(m_params);
         Entity *e : created) {
        if (EntitySpec spec;
            captureEntity(m_scene, e, spec)) {
            m_specs.push_back(std::move(spec));
        }
    }
}

void CreatePatchCommand::undo() {
    m_scene.removeEntities(specIds(m_specs));
}

// ---------------------------------------------------------------------------
// DeleteEntityCommand
// ---------------------------------------------------------------------------

DeleteEntityCommand::DeleteEntityCommand(Scene &scene, const std::vector<EntityId> &ids)
: m_scene(scene) {
    std::vector<EntityId> expandedIds = expandWithPatchControlPoints(scene, ids);
    removeLockedPatchPoints(scene, expandedIds);

    const std::unordered_set deleting(expandedIds.begin(), expandedIds.end());
    auto [specs, deletedPoints] = captureDeleted(scene, expandedIds);
    m_specs = std::move(specs);

    // curves not being deleted that reference a deleted point lose membership
    // via the registry remove-callback; snapshot them so undo can restore exact
    // order
    if (!deletedPoints.empty()) {
        for (const auto &e : scene.getEntities()) {
            if (deleting.contains(e->getId())) {
                continue;
            }
            if (!e->hasComponent<INewPointsTargetBase>()) {
                continue;
            }
            if (const auto cps = currentControlPoints(e.get());
                std::ranges::any_of(
                    cps,
                    [&](const PointHandle h) {
                        return deletedPoints.contains(h);
                    }
                )) {
                m_affectedCurves.push_back({e->getId(), cps});
            }
        }
    }
}

void DeleteEntityCommand::execute() {
    m_scene.removeEntities(specIds(m_specs));
}

void DeleteEntityCommand::undo() {
    // points first so curves can re-reference live handles
    for (const auto &spec : m_specs) {
        if (spec.isPoint()) {
            rebuildEntity(m_scene, spec);
        }
    }
    for (const auto &spec : m_specs) {
        if (!spec.isPoint()) {
            rebuildEntity(m_scene, spec);
        }
    }
    for (const auto &[curveId, controlPoints] : m_affectedCurves) {
        resetMembership(m_scene, curveId, controlPoints);
    }
}

// ---------------------------------------------------------------------------
// TransformCommand
// ---------------------------------------------------------------------------

void TransformCommand::execute() {
    restore(m_after);
}

void TransformCommand::undo() {
    restore(m_before);
}

void TransformCommand::restore(const std::vector<EntitySnapshot> &snaps) const {
    auto &registry = m_scene.getPointRegistry();
    for (const auto &snap : snaps) {
        if (const auto e = m_scene.getEntity(snap.id)) {
            snap.restoreEntity(registry, e.value());
        }
    }
}

// ---------------------------------------------------------------------------
// MovePointCommand
// ---------------------------------------------------------------------------

void MovePointCommand::execute() {
    m_scene.setPointPosition(m_handle, m_after);
}

void MovePointCommand::undo() {
    m_scene.setPointPosition(m_handle, m_before);
}

bool MovePointCommand::tryMerge(const Command &next) {
    const auto *other = dynamic_cast<const MovePointCommand*>(&next);
    if (!other || other->m_handle != m_handle) {
        return false;
    }
    m_after = other->m_after;
    return true;
}

// ---------------------------------------------------------------------------
// RenameCommand
// ---------------------------------------------------------------------------

void RenameCommand::execute() {
    m_scene.setEntityName(m_id, m_after);
}

void RenameCommand::undo() {
    m_scene.setEntityName(m_id, m_before);
}

// ---------------------------------------------------------------------------
// SetPropertyCommand
// ---------------------------------------------------------------------------

bool SetPropertyCommand::tryMerge(const Command &next) {
    const auto *other = dynamic_cast<const SetPropertyCommand*>(&next);
    if (!other || other->m_mergeKey == nullptr || other->m_mergeKey != m_mergeKey) {
        return false;
    }
    m_apply = other->m_apply; // keep our original revert, adopt the newest value
    return true;
}

// ---------------------------------------------------------------------------
// CollapsePointsCommand
// ---------------------------------------------------------------------------

CollapsePointsCommand::CollapsePointsCommand(
    Scene &scene,
    const EntityId keepId,
    const EntityId removeId
)
: m_scene(scene),
  m_keepId(keepId),
  m_removeId(removeId) {
    const auto candidates = scene.validateCollapse(keepId, removeId);
    if (!candidates) {
        return;
    }
    if (!captureEntity(scene, candidates->remove, m_removedSpec)) {
        return;
    }

    m_keepHandle = candidates->keepHandle;
    m_keepPosBefore = scene.getPointRegistry().getPosition(m_keepHandle);

    const PointHandle gone = candidates->removeHandle;
    for (const auto &e : scene.getEntities()) {
        if (const auto r = e->getComponent<IPointReferrer>()) {
            if (const auto cps = r.value()->controlPointHandles();
                std::ranges::find(cps, gone) != cps.end()) {
                m_referrers.push_back({e->getId(), cps});
            }
        }
    }
    m_valid = true;
}

void CollapsePointsCommand::execute() {
    m_valid = m_valid && m_scene.collapsePoints(m_keepId, m_removeId) != nullptr;
}

void CollapsePointsCommand::undo() {
    if (!m_valid) {
        return;
    }
    // point first so referrers can re-lock / re-reference a live handle
    rebuildEntity(m_scene, m_removedSpec);
    m_scene.setPointPosition(m_keepHandle, m_keepPosBefore);
    for (const auto &[entityId, controlPoints] : m_referrers) {
        if (const auto e = m_scene.getEntity(entityId)) {
            if (const auto r = e.value()->getComponent<IPointReferrer>()) {
                r.value()->setControlPointHandles(controlPoints);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// AddControlPointCommand
// ---------------------------------------------------------------------------

void AddControlPointCommand::execute() {
    if (const auto e = m_scene.getEntity(m_curveId)) {
        if (const auto base = e.value()->getComponent<INewPointsTargetBase>()) {
            base.value()->addControlPoint(m_handle);
        }
    }
}

void AddControlPointCommand::undo() {
    if (const auto e = m_scene.getEntity(m_curveId)) {
        if (const auto base = e.value()->getComponent<INewPointsTargetBase>()) {
            base.value()->removeControlPoint(m_handle);
        }
    }
}

// ---------------------------------------------------------------------------
// RemoveControlPointCommand
// ---------------------------------------------------------------------------

RemoveControlPointCommand::RemoveControlPointCommand(
    Scene &scene,
    const EntityId curveId,
    const PointHandle handle
)
: m_scene(scene),
  m_curveId(curveId),
  m_handle(handle) {
    if (const auto e = scene.getEntity(curveId)) {
        m_before = currentControlPoints(e.value());
    }
}

void RemoveControlPointCommand::execute() {
    if (const auto e = m_scene.getEntity(m_curveId)) {
        if (const auto base = e.value()->getComponent<INewPointsTargetBase>()) {
            base.value()->removeControlPoint(m_handle);
        }
    }
}

void RemoveControlPointCommand::undo() {
    resetMembership(m_scene, m_curveId, m_before);
}

namespace {
    std::vector<EntityId> expandWithPatchControlPoints(Scene &scene, const std::vector<EntityId> &ids) {
        std::vector<EntityId> expandedIds = ids;
        std::unordered_set seen(ids.begin(), ids.end());
        for (const auto id : ids) {
            const auto e = scene.getEntity(id);
            if (!e) {
                continue;
            }
            const auto patch = e.value()->getComponent<PatchComponent>();
            if (!patch) {
                continue;
            }
            for (const auto h : patch.value()->getControlPoints()) {
                if (const auto pe = scene.getEntityByPointHandle(h);
                    pe && seen.insert(pe.value()->getId()).second) {
                    expandedIds.push_back(pe.value()->getId());
                }
            }
        }
        return expandedIds;
    }

    std::unordered_map<PointHandle, EntityId> buildPointToPatchMap(const Scene &scene) {
        std::unordered_map<PointHandle, EntityId> pointToPatchMap;
        for (const auto &e : scene.getEntities()) {
            if (const auto patch = e->getComponent<PatchComponent>()) {
                for (const auto h : patch.value()->getControlPoints()) {
                    pointToPatchMap[h] = e->getId();
                }
            }
        }
        return pointToPatchMap;
    }

    void removeLockedPatchPoints(Scene &scene, std::vector<EntityId> &ids) {
        const auto pointToPatchMap = buildPointToPatchMap(scene);
        const std::unordered_set deletingNow(ids.begin(), ids.end());
        std::erase_if(
            ids,
            [&](const EntityId id) {
                const auto e = scene.getEntity(id);
                if (!e) {
                    return false;
                }
                const auto pc = e.value()->getComponent<PointComponent>();
                if (!pc) {
                    return false;
                }
                const auto owningPatch = pointToPatchMap.find(pc.value()->m_handle);
                return owningPatch != pointToPatchMap.end() && !deletingNow.contains(owningPatch->second);
            }
        );
    }

    std::pair<std::vector<EntitySpec>, std::unordered_set<PointHandle>> captureDeleted(
        Scene &scene,
        const std::vector<EntityId> &ids
    ) {
        std::vector<EntitySpec> specs;
        std::unordered_set<PointHandle> deletedPoints;
        for (const auto id : ids) {
            const auto e = scene.getEntity(id);
            if (!e) {
                continue;
            }
            EntitySpec spec;
            if (!captureEntity(scene, e.value(), spec)) {
                continue;
            }
            if (const auto pc = e.value()->getComponent<PointComponent>()) {
                deletedPoints.insert(pc.value()->m_handle);
            }
            specs.push_back(std::move(spec));
        }
        return {std::move(specs), std::move(deletedPoints)};
    }
}
