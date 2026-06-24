//
// Created on 3/19/26.
//

#ifndef CAD_PROJECTIONCAMERASTRATEGY_HPP
#define CAD_PROJECTIONCAMERASTRATEGY_HPP
#include "ICameraStrategy.hpp"

class BlenderCameraStrategy final : public ICameraStrategy {
public:
    explicit BlenderCameraStrategy(
        Entity *cameraEntity,
        const std::function<int()> &widthGetter,
        const std::function<int()> &heightGetter
    );

    cadm::Mat4 getView() override;

    cadm::Mat4 getProjection() override;

    cadm::Mat4 getInvProjection() override;

    void setLookTarget(cadm::Vec3 target) override;

    cadm::cadf distanceToTarget() override;

    bool handleCameraMove(CameraAction action, QPoint delta) override;

    bool handleCameraKeyAction(CameraKeyAction action) override;

    bool handleWheelEvent(QWheelEvent *event) override;

    void toggleProjection() override;

    static constexpr cadm::cadf s_sensitivity = 0.01;
};

#endif //CAD_PROJECTIONCAMERASTRATEGY_HPP
