//
// Created on 5/27/26.
//

#include "CameraController.hpp"

#include "components/PointComponent.hpp"

CameraController::CameraController(QObject *parent) : QObject(parent) {}

void CameraController::addCamera(std::string name, std::unique_ptr<ICameraStrategy> strategy) {
    m_cameras.push_back({std::move(name), std::move(strategy)});
}

void CameraController::removeCamera(const EntityId id) {
    if (m_cameras.size() <= 1) // keep at least one camera
    {
        return;
    }
    const auto it = std::ranges::find_if(
        m_cameras,
        [id](const CameraStrategyEntry &e) {
            return e.strategy->getEntity()->getId() == id;
        }
    );
    if (it == m_cameras.end()) {
        return;
    }
    const auto removedIdx = it - m_cameras.begin();
    m_cameras.erase(it);
    if (m_cameras.empty()) {
        m_activeIndex = 0;
        return;
    }
    if (m_activeIndex >= removedIdx && m_activeIndex > 0) {
        --m_activeIndex;
    }
    m_activeIndex = m_activeIndex % m_cameras.size();
    emit cameraChanged(m_cameras[m_activeIndex].name);
}

ICameraStrategy* CameraController::getActiveStrategy() const {
    assert(!m_cameras.empty() && "getActiveStrategy called with no cameras registered");
    return m_cameras[m_activeIndex].strategy.get();
}

const std::string& CameraController::getActiveName() const {
    assert(!m_cameras.empty() && "getActiveName called with no cameras registered");
    return m_cameras[m_activeIndex].name;
}

void CameraController::switchToNext() {
    if (m_cameras.size() <= 1) {
        return;
    }
    m_activeIndex = (m_activeIndex + 1) % m_cameras.size();
    m_cameras[m_activeIndex].strategy->syncAspectRatio();
    emit cameraChanged(m_cameras[m_activeIndex].name);
}

void CameraController::switchTo(const std::string &name) {
    for (std::size_t i = 0; i < m_cameras.size(); ++i) {
        if (m_cameras[i].name == name) {
            m_activeIndex = i;
            m_cameras[i].strategy->syncAspectRatio();
            emit cameraChanged(name);
            return;
        }
    }
}

void CameraController::switchTo(const EntityId id) {
    for (std::size_t i = 0; i < m_cameras.size(); ++i) {
        if (m_cameras[i].strategy->getEntity()->getId() == id) {
            m_activeIndex = i;
            m_cameras[i].strategy->syncAspectRatio();
            emit cameraChanged(m_cameras[i].name);
            return;
        }
    }
}

bool CameraController::isActiveCamera(const EntityId id) const {
    if (m_cameras.empty()) {
        return false;
    }
    return m_cameras[m_activeIndex].strategy->getEntity()->getId() == id;
}

bool CameraController::isEntityManagedAsCamera(const EntityId id) const {
    return std::ranges::any_of(
        m_cameras,
        [id](const CameraStrategyEntry &e) {
            return e.strategy->getEntity()->getId() == id;
        }
    );
}

void CameraController::lookAtEntity(Entity *entity, const PointRegistry &registry) const {
    auto *strategy = getActiveStrategy();
    if (!strategy || strategy->getEntity() == entity) {
        return;
    }
    if (const auto pc = entity->getComponent<PointComponent>()) {
        strategy->setLookTarget(registry.getPosition(pc.value()->m_handle));
        return;
    }
    const auto transform = entity->getComponent<TransformComponent>();
    if (!transform) {
        return;
    }
    strategy->setLookTarget(transform.value()->getTranslation());
}
