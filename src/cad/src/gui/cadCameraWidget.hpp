//
// Created on 3/23/26.
//

#ifndef CAD_CADCAMERAWIDGET_HPP
#define CAD_CADCAMERAWIDGET_HPP
#include <QFormLayout>

#include "ComponentWidget.h"
#include "../components/cadCameraCompoonent.hpp"


class QDoubleSpinBox;

class cadCameraWidget : public ComponentWidget
{
    Q_OBJECT

public:
    explicit cadCameraWidget(cadCameraComponent *camera, QWidget *parent = nullptr);
    ~cadCameraWidget() override = default;

private
    slots :

    
    void onPositionXChanged(double value) const;
    void onPositionYChanged(double value) const;
    void onPositionZChanged(double value) const;

    void onTargetXChanged(double value) const;
    void onTargetYChanged(double value) const;
    void onTargetZChanged(double value) const;

    void onWorldUpXChanged(double value) const;
    void onWorldUpYChanged(double value) const;
    void onWorldUpZChanged(double value) const;

    void onNearPlaneChanged(double value) const;
    void onFarPlaneChanged(double value) const;
    void onOrthoHeightChanged(double value) const;

private:
    void setUpPositionControls(QFormLayout *layout);
    void setUpTargetControls(QFormLayout *layout);
    void setUpWorldUpControls(QFormLayout *layout);
    void setUpProjectionControls(QFormLayout *layout);

    static constexpr double s_positionMin = cadCameraComponent::s_positionMin;
    static constexpr double s_positionMax = cadCameraComponent::s_positionMax;
    static constexpr double s_positionStep = 0.1;

    static constexpr double s_targetMin = -1000.0;
    static constexpr double s_targetMax = 1000.0;
    static constexpr double s_targetStep = 0.1;

    static constexpr double s_worldUpMin = -1000.0;
    static constexpr double s_worldUpMax = 1000.0;
    static constexpr double s_worldUpStep = 0.1;

    static constexpr double s_nearPlaneMin = cadCameraComponent::s_nearPlaneMin;
    static constexpr double s_nearPlaneMax = cadCameraComponent::s_nearPlaneMax;
    static constexpr double s_farPlaneMin = cadCameraComponent::s_farPlaneMin;
    static constexpr double s_farPlaneMax = cadCameraComponent::s_farPlaneMax;
    static constexpr double s_orthoHeightMin = cadCameraComponent::s_orthoHeightMin;
    static constexpr double s_orthoHeightMax = cadCameraComponent::s_orthoHeightMax;
    static constexpr double s_planeStep = 1.0;
    static constexpr double s_orthoHeightStep = 1.0;

    cadCameraComponent *m_camera;

    QDoubleSpinBox *m_orthoHeight;
    QDoubleSpinBox *m_nearPlane;
    QDoubleSpinBox *m_farPlane;

    QDoubleSpinBox *m_positionX;
    QDoubleSpinBox *m_positionY;
    QDoubleSpinBox *m_positionZ;

    QDoubleSpinBox *m_targetX;
    QDoubleSpinBox *m_targetY;
    QDoubleSpinBox *m_targetZ;

    QDoubleSpinBox *m_worldUpX;
    QDoubleSpinBox *m_worldUpY;
    QDoubleSpinBox *m_worldUpZ;
};


#endif //CAD_CADCAMERAWIDGET_HPP