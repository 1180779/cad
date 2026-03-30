//
// Created on 3/26/26.
//

#ifndef CAD_CAMERACONTROLLER_HPP
#define CAD_CAMERACONTROLLER_HPP

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <QObject>

#include "ICameraStrategy.hpp"
#include "../components/TransformComponent.hpp"

class CameraController : public QObject
{
    Q_OBJECT

public:
    struct CameraEntry
    {
        std::string name;
        std::shared_ptr<ICameraStrategy> strategy;
    };

    explicit CameraController(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void addCamera(std::string name, std::shared_ptr<ICameraStrategy> strategy)
    {
        m_cameras.push_back({std::move(name), std::move(strategy)});
    }

    [[nodiscard]] ICameraStrategy* getActiveStrategy() const
    {
        if (m_cameras.empty())
            return nullptr;
        return m_cameras[m_activeIndex].strategy.get();
    }

    [[nodiscard]] const std::string& getActiveName() const
    {
        return m_cameras[m_activeIndex].name;
    }

    void switchToNext(const int width, const int height)
    {
        if (m_cameras.size() <= 1)
            return;
        m_activeIndex = (m_activeIndex + 1) % m_cameras.size();
        m_cameras[m_activeIndex].strategy->syncAspectRatio(width, height);
        emit cameraChanged(m_cameras[m_activeIndex].name);
    }

    void switchTo(const std::string &name, const int width, const int height)
    {
        for (std::size_t i = 0; i < m_cameras.size(); ++i)
        {
            if (m_cameras[i].name == name)
            {
                m_activeIndex = i;
                m_cameras[i].strategy->syncAspectRatio(width, height);
                emit cameraChanged(name);
                return;
            }
        }
    }

    void switchTo(const EntityID id, const int width, const int height)
    {
        for (std::size_t i = 0; i < m_cameras.size(); ++i)
        {
            if (m_cameras[i].strategy->getEntity()->getId() == id)
            {
                m_activeIndex = i;
                m_cameras[i].strategy->syncAspectRatio(width, height);
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

    void lookAtEntity(Entity *entity) const
    {
        auto *strategy = getActiveStrategy();
        if (!strategy || strategy->getEntity() == entity)
            return;
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
