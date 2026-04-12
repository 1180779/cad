//
// Created on 3/18/26.
//

#ifndef CAD_ICAMERASTRATEGY_HPP
#define CAD_ICAMERASTRATEGY_HPP

#include <QMouseEvent>

#include "../components/CameraComponent.hpp"

enum class CameraAction { Orbit, Pan, ZoomDrag };
enum class CameraKeyAction { MoveUp, MoveDown, MoveLeft, MoveRight };

#include "../components/Entity.hpp"
#include "cad_math/mat4.hpp"

class ICameraStrategy
{
public:
    virtual ~ICameraStrategy() = default;

    explicit ICameraStrategy(Entity *cameraEntity)
        : m_cameraEntity{cameraEntity}
    {
    }

    virtual cadm::mat4 getView() = 0;
    virtual cadm::mat4 getProjection() = 0;
    virtual cadm::mat4 getInvProjection() = 0;
    virtual void setLookTarget(cadm::vec3 target) = 0;
    void syncAspectRatio(int width, int height) const;
    virtual bool handleCameraMove(CameraAction action, QPoint delta) = 0;
    virtual bool handleCameraKeyAction(CameraKeyAction action) = 0;
    virtual bool handleWheelEvent(QWheelEvent *event) = 0;

    // no-op by default; override in strategies that support it
    virtual void toggleProjection()
    {
    }

    [[nodiscard]] Entity* getEntity() const { return m_cameraEntity; }

protected:
    Entity *m_cameraEntity;
    cadm::cadf m_translationStep = 0.1;
};

inline void ICameraStrategy::syncAspectRatio(const int width, const int height) const
{
    const auto cameraComp = m_cameraEntity->getComponent<CameraComponent>();
    if (!cameraComp)
        return;
    cameraComp.value()->setAspectRatio(static_cast<cadm::cadf>(width) / static_cast<cadm::cadf>(height));
}

#endif //CAD_ICAMERASTRATEGY_HPP
