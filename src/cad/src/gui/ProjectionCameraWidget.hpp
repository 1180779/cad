//
// Created on 3/19/26.
//

#ifndef CAD_CAMERAWIDGET_HPP
#define CAD_CAMERAWIDGET_HPP

#include <QFormLayout>
#include <QDoubleSpinBox>

#include "ComponentWidget.hpp"
#include "../components/BlenderCameraComponent.hpp"

class CameraWidget : public ComponentWidget
{
    Q_OBJECT

public:
    explicit CameraWidget(BlenderCameraComponent *camera, QWidget *parent = nullptr);
    ~CameraWidget() override = default;

private
    slots  :



    void onRadiusChanged(double value);

    void onFovChanged(double value) const;
    void onNearPlaneChanged(double value) const;
    void onFarPlaneChanged(double value) const;

    void onTargetXChanged(double value) const;
    void onTargetYChanged(double value) const;
    void onTargetZChanged(double value) const;

private:
    void setUpArcBallControls(QFormLayout *layout);
    void setUpProjectionControls(QFormLayout *layout);
    void setUpTargetControls(QFormLayout *layout);

    static constexpr double s_targetMin = -1000.0;
    static constexpr double s_targetMax = 1000.0;
    static constexpr double s_targetStep = 0.1;

    static constexpr double s_radiusMin = BlenderCameraComponent::s_radiusMin;
    static constexpr double s_radiusMax = BlenderCameraComponent::s_radiusMax;
    static constexpr double s_radiusStep = 0.1;

    static constexpr double s_fovMin = BlenderCameraComponent::s_fovMin / std::numbers::pi * 180.0;
    static constexpr double s_fovMax = BlenderCameraComponent::s_fovMax / std::numbers::pi * 180.0;
    static constexpr double s_fovStep = 1.0;

    static constexpr double s_nearPlaneMin = BlenderCameraComponent::s_nearPlaneMin;
    static constexpr double s_nearPlaneMax = BlenderCameraComponent::s_nearPlaneMax;
    static constexpr double s_farPlaneMin = BlenderCameraComponent::s_farPlaneMin;
    static constexpr double s_farPlaneMax = BlenderCameraComponent::s_farPlaneMax;
    static constexpr double s_planeStep = 1.0;

    BlenderCameraComponent *m_camera;

    QDoubleSpinBox *m_radius;

    QDoubleSpinBox *m_fov;
    QDoubleSpinBox *m_nearPlane;
    QDoubleSpinBox *m_farPlane;

    QDoubleSpinBox *m_targetX;
    QDoubleSpinBox *m_targetY;
    QDoubleSpinBox *m_targetZ;
};


#endif //CAD_CAMERAWIDGET_HPP