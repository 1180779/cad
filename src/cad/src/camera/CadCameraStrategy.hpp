//
// Created on 3/23/26.
//

#ifndef CAD_CADCAMERASTRATEGY_HPP
#define CAD_CADCAMERASTRATEGY_HPP

#include "ICameraStrategy.hpp"

class CadCameraComponent;

class CadCameraStrategy final : public ICameraStrategy {
public:
    explicit CadCameraStrategy(
        Entity *cameraEntity,
        const std::function<int()> &widthGetter,
        const std::function<int()> &heightGetter
    );

    cadm::Mat4 getView() override;

    cadm::Mat4 getProjection() override;

    cadm::Mat4 getInvProjection() override {
        return getProjection().inversedOrtho();
    }

    void setLookTarget(cadm::Vec3 target) override;

    cadm::cadf distanceToTarget() override;

    bool handleCameraMove(CameraAction action, QPoint mouseDelta) override;

    bool handleCameraKeyAction(CameraKeyAction action) override;

    bool handleWheelEvent(QWheelEvent *event) override;

private:
    void handleOrbit(QPoint mouseDelta, CadCameraComponent *pCamera) const;

    void handlePan(QPoint mouseDelta, CadCameraComponent *pCamera) const;
};

#endif //CAD_CADCAMERASTRATEGY_HPP
