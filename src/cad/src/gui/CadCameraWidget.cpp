//
// Created on 3/23/26.
//

#include "CadCameraWidget.hpp"

#include <QDoubleSpinBox>
#include <QLabel>

CadCameraWidget::CadCameraWidget(CadCameraComponent *camera, QWidget *parent) : ComponentWidget{camera, parent},
    m_camera{camera} {
    const auto layout = new QFormLayout(this);
    setUpPositionControls(layout);
    setUpTargetControls(layout);
    setUpWorldUpControls(layout);
    setUpProjectionControls(layout);

    connect(m_camera, &CadCameraComponent::positionXChanged, this, &CadCameraWidget::onPositionXChanged);
    connect(m_camera, &CadCameraComponent::positionYChanged, this, &CadCameraWidget::onPositionYChanged);
    connect(m_camera, &CadCameraComponent::positionZChanged, this, &CadCameraWidget::onPositionZChanged);

    connect(m_camera, &CadCameraComponent::targetXChanged, this, &CadCameraWidget::onTargetXChanged);
    connect(m_camera, &CadCameraComponent::targetYChanged, this, &CadCameraWidget::onTargetYChanged);
    connect(m_camera, &CadCameraComponent::targetZChanged, this, &CadCameraWidget::onTargetZChanged);

    connect(m_camera, &CadCameraComponent::worldUpXChanged, this, &CadCameraWidget::onWorldUpXChanged);
    connect(m_camera, &CadCameraComponent::worldUpYChanged, this, &CadCameraWidget::onWorldUpYChanged);
    connect(m_camera, &CadCameraComponent::worldUpZChanged, this, &CadCameraWidget::onWorldUpZChanged);

    connect(m_camera, &CadCameraComponent::nearPlaneChanged, this, &CadCameraWidget::onNearPlaneChanged);
    connect(m_camera, &CadCameraComponent::farPlaneChanged, this, &CadCameraWidget::onFarPlaneChanged);
    connect(m_camera, &CadCameraComponent::orthoHeightChanged, this, &CadCameraWidget::onOrthoHeightChanged);

    connect(m_camera, &CadCameraComponent::propertyUpdated, this, &CadCameraWidget::propertyChanged);
}

void CadCameraWidget::onPositionXChanged(const double value) const {
    m_positionX->blockSignals(true);
    m_positionX->setValue(value);
    m_positionX->blockSignals(false);
}

void CadCameraWidget::onPositionYChanged(const double value) const {
    m_positionY->blockSignals(true);
    m_positionY->setValue(value);
    m_positionY->blockSignals(false);
}

void CadCameraWidget::onPositionZChanged(const double value) const {
    m_positionZ->blockSignals(true);
    m_positionZ->setValue(value);
    m_positionZ->blockSignals(false);
}

void CadCameraWidget::onTargetXChanged(const double value) const {
    m_targetX->blockSignals(true);
    m_targetX->setValue(value);
    m_targetX->blockSignals(false);
}

void CadCameraWidget::onTargetYChanged(const double value) const {
    m_targetY->blockSignals(true);
    m_targetY->setValue(value);
    m_targetY->blockSignals(false);
}

void CadCameraWidget::onTargetZChanged(const double value) const {
    m_targetZ->blockSignals(true);
    m_targetZ->setValue(value);
    m_targetZ->blockSignals(false);
}

void CadCameraWidget::onWorldUpXChanged(const double value) const {
    m_worldUpX->blockSignals(true);
    m_worldUpX->setValue(value);
    m_worldUpX->blockSignals(false);
}

void CadCameraWidget::onWorldUpYChanged(const double value) const {
    m_worldUpY->blockSignals(true);
    m_worldUpY->setValue(value);
    m_worldUpY->blockSignals(false);
}

void CadCameraWidget::onWorldUpZChanged(const double value) const {
    m_worldUpZ->blockSignals(true);
    m_worldUpZ->setValue(value);
    m_worldUpZ->blockSignals(false);
}

void CadCameraWidget::onNearPlaneChanged(const double value) const {
    m_nearPlane->blockSignals(true);
    m_nearPlane->setValue(value);
    m_nearPlane->blockSignals(false);
}

void CadCameraWidget::onFarPlaneChanged(const double value) const {
    m_farPlane->blockSignals(true);
    m_farPlane->setValue(value);
    m_farPlane->blockSignals(false);
}

void CadCameraWidget::onOrthoHeightChanged(const double value) const {
    m_orthoHeight->blockSignals(true);
    m_orthoHeight->setValue(value);
    m_orthoHeight->blockSignals(false);
}

void CadCameraWidget::setUpPositionControls(QFormLayout *layout) {
    m_positionX = new QDoubleSpinBox();
    m_positionY = new QDoubleSpinBox();
    m_positionZ = new QDoubleSpinBox();

    m_positionX->setRange(s_positionMin, s_positionMax);
    m_positionY->setRange(s_positionMin, s_positionMax);
    m_positionZ->setRange(s_positionMin, s_positionMax);

    m_positionX->setSingleStep(s_positionStep);
    m_positionY->setSingleStep(s_positionStep);
    m_positionZ->setSingleStep(s_positionStep);

    m_positionX->setValue(m_camera->getPositionX());
    m_positionY->setValue(m_camera->getPositionY());
    m_positionZ->setValue(m_camera->getPositionZ());

    m_positionX->setKeyboardTracking(true);
    m_positionY->setKeyboardTracking(true);
    m_positionZ->setKeyboardTracking(true);

    m_positionX->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_positionY->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_positionZ->setFixedWidth(s_doubleSpinBoxFixedWidth);

    connect(
        m_positionX,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setPositionX
    );
    connect(
        m_positionY,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setPositionY
    );
    connect(
        m_positionZ,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setPositionZ
    );

    const auto targetLayout = new QHBoxLayout();
    targetLayout->addWidget(m_positionX);
    targetLayout->addWidget(m_positionY);
    targetLayout->addWidget(m_positionZ);
    layout->addRow(new QLabel("Position"), targetLayout);
}

void CadCameraWidget::setUpTargetControls(QFormLayout *layout) {
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
        &CadCameraComponent::setTargetX
    );
    connect(
        m_targetY,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setTargetY
    );
    connect(
        m_targetZ,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setTargetZ
    );

    const auto targetLayout = new QHBoxLayout();
    targetLayout->addWidget(m_targetX);
    targetLayout->addWidget(m_targetY);
    targetLayout->addWidget(m_targetZ);
    layout->addRow(new QLabel("Target"), targetLayout);
}

void CadCameraWidget::setUpWorldUpControls(QFormLayout *layout) {
    m_worldUpX = new QDoubleSpinBox();
    m_worldUpY = new QDoubleSpinBox();
    m_worldUpZ = new QDoubleSpinBox();

    m_worldUpX->setRange(s_worldUpMin, s_worldUpMax);
    m_worldUpY->setRange(s_worldUpMin, s_worldUpMax);
    m_worldUpZ->setRange(s_worldUpMin, s_worldUpMax);

    m_worldUpX->setSingleStep(s_worldUpStep);
    m_worldUpY->setSingleStep(s_worldUpStep);
    m_worldUpZ->setSingleStep(s_worldUpStep);

    m_worldUpX->setValue(m_camera->getWorldUpX());
    m_worldUpY->setValue(m_camera->getWorldUpY());
    m_worldUpZ->setValue(m_camera->getWorldUpZ());

    m_worldUpX->setKeyboardTracking(true);
    m_worldUpY->setKeyboardTracking(true);
    m_worldUpZ->setKeyboardTracking(true);

    m_worldUpX->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_worldUpY->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_worldUpZ->setFixedWidth(s_doubleSpinBoxFixedWidth);

    connect(
        m_worldUpX,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setWorldUpX
    );
    connect(
        m_worldUpY,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setWorldUpY
    );
    connect(
        m_worldUpZ,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setWorldUpZ
    );

    const auto worldUpLayout = new QHBoxLayout();
    worldUpLayout->addWidget(m_worldUpX);
    worldUpLayout->addWidget(m_worldUpY);
    worldUpLayout->addWidget(m_worldUpZ);
    layout->addRow(new QLabel("WorldUp"), worldUpLayout);
}

void CadCameraWidget::setUpProjectionControls(QFormLayout *layout) {
    m_orthoHeight = new QDoubleSpinBox();
    m_orthoHeight->setRange(s_orthoHeightMin, s_orthoHeightMax);
    m_orthoHeight->setSingleStep(s_orthoHeightStep);
    m_orthoHeight->setValue(m_camera->getOrthoHeight());
    m_orthoHeight->setFixedWidth(s_doubleSpinBoxFixedWidth);
    connect(
        m_orthoHeight,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this,
        [this](const double value) {
            m_camera->setOrthoHeight(value);
        }
    );
    layout->addRow("OrthoHeight", m_orthoHeight);

    m_nearPlane = new QDoubleSpinBox();
    m_nearPlane->setRange(s_nearPlaneMin, s_nearPlaneMax);
    m_nearPlane->setSingleStep(s_planeStep);
    m_nearPlane->setValue(m_camera->getNearPlane());
    m_nearPlane->setFixedWidth(s_doubleSpinBoxFixedWidth);
    connect(
        m_nearPlane,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setNearPlane
    );
    layout->addRow("Near Plane", m_nearPlane);

    m_farPlane = new QDoubleSpinBox();
    m_farPlane->setRange(s_farPlaneMin, s_farPlaneMax);
    m_farPlane->setSingleStep(s_planeStep);
    m_farPlane->setValue(m_camera->getFarPlane());
    m_farPlane->setFixedWidth(s_doubleSpinBoxFixedWidth);
    connect(
        m_farPlane,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        m_camera,
        &CadCameraComponent::setFarPlane
    );
    layout->addRow("Far Plane", m_farPlane);
}
