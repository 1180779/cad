//
// Created on 3/19/26.
//

#include "cameraWidget.hpp"

#include <QGroupBox>
#include <QLabel>
#include <QHBoxLayout>
#include <numbers>

CameraWidget::CameraWidget(CameraComponent *camera, QWidget *parent)
    : ComponentWidget(camera, parent), m_camera(camera)
{
    const auto layout = new QFormLayout(this);
    setUpArcBallControls(layout);
    setUpTargetControls(layout);
    setUpProjectionControls(layout);

    connect(
        m_camera,
        &CameraComponent::radiusChanged,
        this,
        &CameraWidget::onRadiusChanged);
    connect(
        m_camera,
        &CameraComponent::azimuthAngleChanged,
        this,
        &CameraWidget::onAzimuthAngleChanged);
    connect(
        m_camera,
        &CameraComponent::polarAngleChanged,
        this,
        &CameraWidget::onPolarAngleChanged);
    connect(
        m_camera,
        &CameraComponent::fovChanged,
        this,
        &CameraWidget::onFovChanged);
    connect(
        m_camera,
        &CameraComponent::nearPlaneChanged,
        this,
        &CameraWidget::onNearPlaneChanged);
    connect(
        m_camera,
        &CameraComponent::farPlaneChanged,
        this,
        &CameraWidget::onFarPlaneChanged);

    connect(
        m_camera,
        &CameraComponent::targetXChanged,
        this,
        &CameraWidget::onTargetXChanged);
    connect(
        m_camera,
        &CameraComponent::targetYChanged,
        this,
        &CameraWidget::onTargetYChanged);
    connect(
        m_camera,
        &CameraComponent::targetZChanged,
        this,
        &CameraWidget::onTargetZChanged);

    connect(
        m_camera,
        &CameraComponent::propertyUpdated,
        this,
        &ComponentWidget::propertyChanged);
}

void CameraWidget::onRadiusChanged(const double value)
{
    m_radius->blockSignals(true);
    m_radius->setValue(value);
    m_radius->blockSignals(false);
    emit propertyChanged();
}

void CameraWidget::onAzimuthAngleChanged(const double value) const
{
    m_azimuthAngle->blockSignals(true);
    m_azimuthAngle->setValue(value * 180.0 / std::numbers::pi);
    m_azimuthAngle->blockSignals(false);
}

void CameraWidget::onPolarAngleChanged(const double value) const
{
    m_polarAngle->blockSignals(true);
    m_polarAngle->setValue(value * 180.0 / std::numbers::pi);
    m_polarAngle->blockSignals(false);
}

void CameraWidget::onFovChanged(const double value) const
{
    m_fov->blockSignals(true);
    m_fov->setValue(value * 180.0 / std::numbers::pi);
    m_fov->blockSignals(false);
}


void CameraWidget::onNearPlaneChanged(const double value) const
{
    m_nearPlane->blockSignals(true);
    m_nearPlane->setValue(value);
    m_nearPlane->blockSignals(false);
}

void CameraWidget::onFarPlaneChanged(const double value) const
{
    m_farPlane->blockSignals(true);
    m_farPlane->setValue(value);
    m_farPlane->blockSignals(false);
}

void CameraWidget::onTargetXChanged(const double value) const
{
    m_targetX->blockSignals(true);
    m_targetX->setValue(value);
    m_targetX->blockSignals(false);
}

void CameraWidget::onTargetYChanged(const double value) const
{
    m_targetY->blockSignals(true);
    m_targetY->setValue(value);
    m_targetY->blockSignals(false);
}

void CameraWidget::onTargetZChanged(const double value) const
{
    m_targetZ->blockSignals(true);
    m_targetZ->setValue(value);
    m_targetZ->blockSignals(false);
}

void CameraWidget::setUpArcBallControls(QFormLayout *layout)
{
    m_radius = new QDoubleSpinBox();
    m_radius->setRange(s_radiusMin, s_radiusMax);
    m_radius->setSingleStep(s_radiusStep);
    m_radius->setValue(m_camera->getRadius());
    connect(
        m_radius,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CameraComponent::setRadius);
    layout->addRow("Radius", m_radius);

    m_azimuthAngle = new QDoubleSpinBox();
    m_azimuthAngle->setRange(s_azimuthAngleMin, s_azimuthAngleMax);
    m_azimuthAngle->setSingleStep(s_azimuthAngleStep);
    m_azimuthAngle->setValue(m_camera->getAzimuthAngle() * 180.0 / std::numbers::pi);
    m_azimuthAngle->setWrapping(true);
    connect(
        m_azimuthAngle,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this,
        [this](const double value)
        {
            m_camera->setAzimuthAngle(value * std::numbers::pi / 180.0);
        });
    layout->addRow("Azimuth Angle", m_azimuthAngle);

    m_polarAngle = new QDoubleSpinBox();
    m_polarAngle->setRange(s_polarAngleMin, s_polarAngleMax);
    m_polarAngle->setSingleStep(s_polarAngleStep);
    m_polarAngle->setValue(m_camera->getPolarAngle() * 180.0 / std::numbers::pi);
    connect(
        m_polarAngle,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this,
        [this](const double value)
        {
            m_camera->setPolarAngle(value * std::numbers::pi / 180.0);
        });
    layout->addRow("Polar Angle", m_polarAngle);
}

void CameraWidget::setUpProjectionControls(QFormLayout *layout)
{
    m_fov = new QDoubleSpinBox();
    m_fov->setRange(s_fovMin, s_fovMax);
    m_fov->setSingleStep(s_fovStep);
    m_fov->setValue(m_camera->getFov() * 180.0 / std::numbers::pi);
    connect(
        m_fov,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this,
        [this](const double value)
        {
            m_camera->setFov(value * std::numbers::pi / 180.0);
        });
    layout->addRow("FOV", m_fov);


    m_nearPlane = new QDoubleSpinBox();
    m_nearPlane->setRange(s_nearPlaneMin, s_nearPlaneMax);
    m_nearPlane->setSingleStep(s_planeStep);
    m_nearPlane->setValue(m_camera->getNearPlane());
    connect(
        m_nearPlane,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CameraComponent::setNearPlane);
    layout->addRow("Near Plane", m_nearPlane);

    m_farPlane = new QDoubleSpinBox();
    m_farPlane->setRange(s_farPlaneMin, s_farPlaneMax);
    m_farPlane->setSingleStep(s_planeStep);
    m_farPlane->setValue(m_camera->getFarPlane());
    connect(
        m_farPlane,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CameraComponent::setFarPlane);
    layout->addRow("Far Plane", m_farPlane);
}

void CameraWidget::setUpTargetControls(QFormLayout *layout)
{
    m_targetX = new QDoubleSpinBox();
    m_targetY = new QDoubleSpinBox();
    m_targetZ = new QDoubleSpinBox();

    m_targetX->setRange(s_targetMin, s_targetMax);
    m_targetY->setRange(s_targetMin, s_targetMax);
    m_targetZ->setRange(s_targetMin, s_targetMax);

    m_targetX->setSingleStep(s_targetStep);
    m_targetY->setSingleStep(s_targetStep);
    m_targetZ->setSingleStep(s_targetStep);

    m_targetX->setValue(m_camera->getTargetX());
    m_targetY->setValue(m_camera->getTargetY());
    m_targetZ->setValue(m_camera->getTargetZ());

    m_targetX->setKeyboardTracking(true);
    m_targetY->setKeyboardTracking(true);
    m_targetZ->setKeyboardTracking(true);

    m_targetX->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_targetY->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_targetZ->setFixedWidth(s_doubleSpinBoxFixedWidth);

    connect(
        m_targetX,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CameraComponent::setTargetX);
    connect(
        m_targetY,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CameraComponent::setTargetY);
    connect(
        m_targetZ,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CameraComponent::setTargetZ);

    const auto targetLayout = new QHBoxLayout();
    targetLayout->addWidget(m_targetX);
    targetLayout->addWidget(m_targetY);
    targetLayout->addWidget(m_targetZ);
    layout->addRow(new QLabel("Target"), targetLayout);
}
