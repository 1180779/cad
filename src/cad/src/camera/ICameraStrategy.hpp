//
// Created on 3/18/26.
//

#ifndef CAD_ICAMERASTRATEGY_HPP
#define CAD_ICAMERASTRATEGY_HPP

#include <QMouseEvent>

#include "../components/camera.hpp"
#include "../entities/entity.h"
#include "cad_math/mat4.h"

class ICameraStrategy
{
public:
    virtual ~ICameraStrategy() = default;

    explicit ICameraStrategy(entity *cameraEntity)
        : m_cameraEntity{cameraEntity}
    {
    }

    virtual cadm::mat4 getView() = 0;
    virtual cadm::mat4 getProjection() = 0;
    void syncAspectRatio(int width, int height) const;
    virtual bool handleMousePressEvent(QMouseEvent *event) = 0;
    virtual bool handleMouseMoveEvent(QMouseEvent *event) = 0;
    virtual bool handleKeyPressEvent(QKeyEvent *event) = 0;
    virtual bool handleWheelEvent(QWheelEvent *event) = 0;

protected:
    entity *m_cameraEntity;
    cadm::cadf m_translationStep = 0.1;
};

inline void ICameraStrategy::syncAspectRatio(const int width, const int height) const
{
    const auto cameraComp = m_cameraEntity->getComponent<CameraComponent>();
    if (!cameraComp)
        return;
    cameraComp.value()->m_aspectRatio = static_cast<cadm::cadf>(width) / static_cast<cadm::cadf>(height);
}

#endif //CAD_ICAMERASTRATEGY_HPP
