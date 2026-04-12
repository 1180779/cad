//
// Created on 3/26/26.
//

#ifndef CAD_CAMERACONTROLLER_HPP
#define CAD_CAMERACONTROLLER_HPP

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include <QObject>

#include "ICameraStrategy.hpp"
#include "../components/PointComponent.hpp"
#include "../components/TransformComponent.hpp"
#include "../PointRegistry.hpp"

class CameraController : public QObject
{
    Q_OBJECT

public:
    struct CameraEntry
    {
        std::string name;
        std::unique_ptr<ICameraStrategy> strategy;
    };

    explicit CameraController(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void addCamera(std::string name, std::unique_ptr<ICameraStrategy> strategy)
    {
        m_cameras.push_back({std::move(name), std::move(strategy)});
    }

    void removeCamera(const EntityID id)
    {
        if (m_cameras.size() <= 1) // keep at least one camera
            return;
        const auto it = std::ranges::find_if(
            m_cameras,
            [id](const CameraEntry &e) { return e.strategy->getEntity()->getId() == id; });
        if (it == m_cameras.end())
            return;
        const auto removedIdx = it - m_cameras.begin();
        m_cameras.erase(it);
        if (m_cameras.empty())
        {
            m_activeIndex = 0;
            return;
        }
        if (m_activeIndex >= removedIdx && m_activeIndex > 0)
            --m_activeIndex;
        m_activeIndex = m_activeIndex % m_cameras.size();
        emit cameraChanged(m_cameras[m_activeIndex].name);
    }

    [[nodiscard]] ICameraStrategy* getActiveStrategy() const
    {
        assert(!m_cameras.empty() && "getActiveStrategy called with no cameras registered");
        return m_cameras[m_activeIndex].strategy.get();
    }

    [[nodiscard]] const std::string& getActiveName() const
    {
        assert(!m_cameras.empty() && "getActiveName called with no cameras registered");
        return m_cameras[m_activeIndex].name;
    }

    void switchToNext()
    {
        if (m_cameras.size() <= 1)
            return;
        m_activeIndex = (m_activeIndex + 1) % m_cameras.size();
        m_cameras[m_activeIndex].strategy->syncAspectRatio();
        emit cameraChanged(m_cameras[m_activeIndex].name);
    }

    void switchTo(const std::string &name)
    {
        for (std::size_t i = 0; i < m_cameras.size(); ++i)
        {
            if (m_cameras[i].name == name)
            {
                m_activeIndex = i;
                m_cameras[i].strategy->syncAspectRatio();
                emit cameraChanged(name);
                return;
            }
        }
    }

    void switchTo(const EntityID id)
    {
        for (std::size_t i = 0; i < m_cameras.size(); ++i)
        {
            if (m_cameras[i].strategy->getEntity()->getId() == id)
            {
                m_activeIndex = i;
                m_cameras[i].strategy->syncAspectRatio();
                emit cameraChanged(m_cameras[i].name);
                return;
            }
        }
    }

    [[nodiscard]] bool isActiveCamera(const EntityID id) const
    {
        if (m_cameras.empty()) return false;
        return m_cameras[m_activeIndex].strategy->getEntity()->getId() == id;
    }

    [[nodiscard]] bool isManagedCamera(const EntityID id) const
    {
        return std::ranges::any_of(
            m_cameras,
            [id](const CameraEntry &e)
            {
                return e.strategy->getEntity()->getId() == id;
            });
    }

    void lookAtEntity(Entity *entity, const PointRegistry &registry) const
    {
        auto *strategy = getActiveStrategy();
        if (!strategy || strategy->getEntity() == entity)
            return;
        if (const auto pc = entity->getComponent<PointComponent>())
        {
            strategy->setLookTarget(registry.getPosition(pc.value()->m_handle));
            return;
        }
        const auto transform = entity->getComponent<TransformComponent>();
        if (!transform)
            return;
        strategy->setLookTarget(transform.value()->getTranslation());
    }

    [[nodiscard]] const std::vector<CameraEntry>& cameras() const { return m_cameras; }

signals:
    void cameraChanged(const std::string &name);

private:
    std::vector<CameraEntry> m_cameras;
    std::size_t m_activeIndex{0};
};

#endif //CAD_CAMERACONTROLLER_HPP
