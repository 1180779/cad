//
// Created on 3/19/26.
//

#ifndef CAD_PROJECTIONCAMERASTRATEGY_HPP
#define CAD_PROJECTIONCAMERASTRATEGY_HPP
#include "ICameraStrategy.hpp"


class projectionCameraStrategy : public ICameraStrategy
{
public:
    explicit projectionCameraStrategy(
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

    [[nodiscard]] cadm::cadf getZoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(cadm::cadf zoomFactor);

    static constexpr cadm::cadf s_sensitivity = 0.01;

private:
    std::function<int()> m_widthGetter;
    std::function<int()> m_heightGetter;
    cadm::cadf m_zoomFactor = 1.1;
    bool m_mousePressed{false};
};


#endif //CAD_PROJECTIONCAMERASTRATEGY_HPP
