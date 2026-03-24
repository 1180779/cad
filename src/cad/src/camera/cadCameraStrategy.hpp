//
// Created on 3/23/26.
//

#ifndef CAD_CADCAMERASTRATEGY_HPP
#define CAD_CADCAMERASTRATEGY_HPP

#include "ICameraStrategy.hpp"

class CadCameraStrategy final : public ICameraStrategy
{
public:
    explicit CadCameraStrategy(
        entity *cameraEntity,
        const std::function<int()> &widthGetter,
        const std::function<int()> &heightGetter);

    cadm::mat4 getView() override;
    cadm::mat4 getProjection() override;
    bool handleMouseMoveEvent(QMouseEvent *event, QPoint mouseDelta) override;
    bool handleMousePressEvent(QMouseEvent *event) override;
    bool handleMouseReleaseEvent(QMouseEvent *event) override;
    bool handleKeyPressEvent(QKeyEvent *event) override;
    bool handleWheelEvent(QWheelEvent *event) override;

private:
    std::function<int()> m_widthGetter;
    std::function<int()> m_heightGetter;
    bool m_leftMouseDown{false};
    bool m_rightMouseDown{false};
};

#endif //CAD_CADCAMERASTRATEGY_HPP