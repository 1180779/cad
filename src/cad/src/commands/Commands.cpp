//
// Created on 6/19/26.
//

#include "Commands.hpp"

#include <algorithm>
#include <unordered_set>

#include "../Scene.hpp"
#include "../components/BezierC0Component.hpp"
#include "../components/BezierC2Component.hpp"
#include "../components/INewPointsTargetComponent.hpp"
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
        return {};
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
// DeleteEntityCommand
// ---------------------------------------------------------------------------

DeleteEntityCommand::DeleteEntityCommand(Scene &scene, const std::vector<EntityId> &ids) : m_scene(scene) {
    const std::unordered_set deleting(ids.begin(), ids.end());
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
        m_specs.push_back(std::move(spec));
    }

    // curves not being deleted that reference a deleted point lose membership via
    // the registry remove-callback; snapshot them so undo can restore exact order
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
    for (const auto &spec : m_specs) {
        m_scene.removeEntity(spec.id);
    }
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

void TransformCommand::restore(const std::vector<EntitySnapshot> &snaps) const {
    auto &registry = m_scene.getPointRegistry();
    for (const auto &snap : snaps) {
        if (const auto e = m_scene.getEntity(snap.id)) {
            snap.restoreEntity(registry, e.value());
        }
    }
}

void TransformCommand::execute() {
    restore(m_after);
}

void TransformCommand::undo() {
    restore(m_before);
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
) : m_scene(scene), m_curveId(curveId), m_handle(handle) {
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
