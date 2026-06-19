//
// Created on 3/18/26.
//

#ifndef CAD_ICAMERASTRATEGY_HPP
#define CAD_ICAMERASTRATEGY_HPP

#include <functional>
#include <QMouseEvent>

#include "../components/CameraComponent.hpp"

enum class CameraAction { orbit, pan, zoomDrag };

enum class CameraKeyAction { moveUp, moveDown, moveLeft, moveRight };

#include "../components/Entity.hpp"
#include "cad_math/mat4.hpp"

class ICameraStrategy {
public:
    virtual ~ICameraStrategy() = default;

    ICameraStrategy(
        Entity *cameraEntity,
        std::function<int()> widthGetter,
        std::function<int()> heightGetter
    ) : m_cameraEntity{cameraEntity}, m_widthGetter{std::move(widthGetter)}, m_heightGetter{std::move(heightGetter)} {}

    virtual cadm::mat4 getView() = 0;

    virtual cadm::mat4 getProjection() = 0;

    virtual cadm::mat4 getInvProjection() = 0;

    virtual void setLookTarget(cadm::vec3 target) = 0;

    void syncAspectRatio() const;

    virtual bool handleCameraMove(CameraAction action, QPoint delta) = 0;

    virtual bool handleCameraKeyAction(CameraKeyAction action) = 0;

    virtual bool handleWheelEvent(QWheelEvent *event) = 0;

    // no-op by default; override in strategies that support it
    virtual void toggleProjection() {}

    [[nodiscard]] Entity* getEntity() const {
        return m_cameraEntity;
    }

protected:
    Entity *m_cameraEntity;
    std::function<int()> m_widthGetter;
    std::function<int()> m_heightGetter;
    cadm::cadf m_translationStep = 0.1;
};

inline void ICameraStrategy::syncAspectRatio() const {
    const auto cameraComp = m_cameraEntity->getComponent<CameraComponent>();
    if (!cameraComp) {
        return;
    }
    cameraComp.value()->setAspectRatio(
        static_cast<cadm::cadf>(m_widthGetter()) / static_cast<cadm::cadf>(m_heightGetter())
    );
}

#endif //CAD_ICAMERASTRATEGY_HPP
