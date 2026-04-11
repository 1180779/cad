//
// Created on 3/19/26.
//

#ifndef CAD_PROJECTIONCAMERASTRATEGY_HPP
#define CAD_PROJECTIONCAMERASTRATEGY_HPP
#include "ICameraStrategy.hpp"


class ProjectionCameraStrategy : public ICameraStrategy
{
public:
    explicit ProjectionCameraStrategy(
        Entity *cameraEntity,
        const std::function<int()> &widthGetter,
        const std::function<int()> &heightGetter);

    cadm::mat4 getView() override;
    cadm::mat4 getProjection() override;
    cadm::mat4 getInvProjection() override { return getProjection().inversedProjectionMO(); }
    void setLookTarget(cadm::vec3 target) override;
    bool handleCameraMove(CameraAction action, QPoint delta) override;
    bool handleKeyPressEvent(QKeyEvent *event) override;
    bool handleWheelEvent(QWheelEvent *event) override;

    static constexpr cadm::cadf s_sensitivity = 0.01;

private:
    std::function<int()> m_widthGetter;
    std::function<int()> m_heightGetter;
};


#endif //CAD_PROJECTIONCAMERASTRATEGY_HPP
