//
// Created on 3/18/26.
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
    bool handleMouseMoveEvent(QMouseEvent *event) override;
    bool handleMousePressEvent(QMouseEvent *event) override;
    bool handleKeyPressEvent(QKeyEvent *event) override;
    bool handleWheelEvent(QWheelEvent *event) override;

    cadm::cadf getZoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(cadm::cadf zoomFactor);

private:
    std::function<int()> m_widthGetter;
    std::function<int()> m_heightGetter;
    cadm::cadf m_zoomFactor = 1.1;
};


#endif //CAD_CADCAMERASTRATEGY_HPP
