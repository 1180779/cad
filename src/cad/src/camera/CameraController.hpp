//
// Created on 3/26/26.
//

#ifndef CAD_CAMERACONTROLLER_HPP
#define CAD_CAMERACONTROLLER_HPP

#include <memory>
#include <string>
#include <vector>

#include <QObject>

#include "ICameraStrategy.hpp"

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

    [[nodiscard]] const std::vector<CameraEntry>& cameras() const { return m_cameras; }

signals:
    void cameraChanged(const std::string &name);

private:
    std::vector<CameraEntry> m_cameras;
    std::size_t m_activeIndex{0};
};

#endif //CAD_CAMERACONTROLLER_HPP
