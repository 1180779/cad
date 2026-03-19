#include "TransformWidget.h"
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLabel>
#include <limits>
#include <numbers>

TransformWidget::TransformWidget(TransformComponent *transform, QWidget *parent)
    : ComponentWidget(transform, parent), m_transform(transform)
{
    const auto layout = new QFormLayout(this);
    setUpTranslationControls(layout);
    setUpScaleControls(layout);
    setUpRotationControls(layout);
}

void TransformWidget::onTranslationXChanged(const double value) const
{
    auto t = m_transform->getTranslation();
    m_transform->setTranslation({static_cast<cadm::cadf>(value), t.y, t.z});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::onTranslationYChanged(const double value) const
{
    auto t = m_transform->getTranslation();
    m_transform->setTranslation({t.x, static_cast<cadm::cadf>(value), t.z});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::onTranslationZChanged(const double value) const
{
    auto t = m_transform->getTranslation();
    m_transform->setTranslation({t.x, t.y, static_cast<cadm::cadf>(value)});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::onScaleXChanged(const double value) const
{
    auto s = m_transform->getScale();
    m_transform->setScale({static_cast<cadm::cadf>(value), s.y, s.z});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::onScaleYChanged(const double value) const
{
    auto s = m_transform->getScale();
    m_transform->setScale({s.x, static_cast<cadm::cadf>(value), s.z});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::onScaleZChanged(const double value) const
{
    auto s = m_transform->getScale();
    m_transform->setScale({s.x, s.y, static_cast<cadm::cadf>(value)});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::onRotationXChanged(const double value) const
{
    auto r = m_transform->getRotation();
    m_transform->setRotation({static_cast<cadm::cadf>(value * std::numbers::pi / 180), r.y, r.z});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::onRotationYChanged(const double value) const
{
    auto r = m_transform->getRotation();
    m_transform->setRotation({r.x, static_cast<cadm::cadf>(value * std::numbers::pi / 180), r.z});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::onRotationZChanged(const double value) const
{
    auto r = m_transform->getRotation();
    m_transform->setRotation({r.x, r.y, static_cast<cadm::cadf>(value * std::numbers::pi / 180)});
    emit const_cast<TransformWidget*>(this)->propertyChanged();
}

void TransformWidget::setUpTranslationControls(QFormLayout *const layout)
{
    m_translationX = new QDoubleSpinBox();
    m_translationY = new QDoubleSpinBox();
    m_translationZ = new QDoubleSpinBox();

    m_translationX->setSingleStep(s_translationStep);
    m_translationY->setSingleStep(s_translationStep);
    m_translationZ->setSingleStep(s_translationStep);

    m_translationX->setRange(s_translationMin, s_translationMax);
    m_translationY->setRange(s_translationMin, s_translationMax);
    m_translationZ->setRange(s_translationMin, s_translationMax);

    m_translationX->setValue(m_transform->getTranslation().x);
    m_translationY->setValue(m_transform->getTranslation().y);
    m_translationZ->setValue(m_transform->getTranslation().z);

    m_translationX->setKeyboardTracking(true);
    m_translationY->setKeyboardTracking(true);
    m_translationZ->setKeyboardTracking(true);

    m_translationX->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_translationY->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_translationZ->setFixedWidth(s_doubleSpinBoxFixedWidth);

    connect(m_translationX, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onTranslationXChanged);
    connect(m_translationY, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onTranslationYChanged);
    connect(m_translationZ, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onTranslationZChanged);
    const auto translationLayout = new QHBoxLayout();
    translationLayout->addWidget(m_translationX);
    translationLayout->addWidget(m_translationY);
    translationLayout->addWidget(m_translationZ);
    layout->addRow(new QLabel("Translation"), translationLayout);
}

void TransformWidget::setUpScaleControls(QFormLayout *const layout)
{
    m_scaleX = new QDoubleSpinBox();
    m_scaleY = new QDoubleSpinBox();
    m_scaleZ = new QDoubleSpinBox();

    m_scaleX->setSingleStep(s_scaleStep);
    m_scaleY->setSingleStep(s_scaleStep);
    m_scaleZ->setSingleStep(s_scaleStep);

    m_scaleX->setRange(s_scaleMin, s_scaleMax);
    m_scaleY->setRange(s_scaleMin, s_scaleMax);
    m_scaleZ->setRange(s_scaleMin, s_scaleMax);

    m_scaleX->setValue(m_transform->getScale().x);
    m_scaleY->setValue(m_transform->getScale().y);
    m_scaleZ->setValue(m_transform->getScale().z);

    m_scaleX->setKeyboardTracking(true);
    m_scaleY->setKeyboardTracking(true);
    m_scaleZ->setKeyboardTracking(true);

    m_scaleX->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_scaleY->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_scaleZ->setFixedWidth(s_doubleSpinBoxFixedWidth);

    connect(m_scaleX, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onScaleXChanged);
    connect(m_scaleY, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onScaleYChanged);
    connect(m_scaleZ, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onScaleZChanged);
    const auto scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(m_scaleX);
    scaleLayout->addWidget(m_scaleY);
    scaleLayout->addWidget(m_scaleZ);
    layout->addRow(new QLabel("Scale"), scaleLayout);
}

void TransformWidget::setUpRotationControls(QFormLayout *const layout)
{
    m_rotationX = new QDoubleSpinBox();
    m_rotationY = new QDoubleSpinBox();
    m_rotationZ = new QDoubleSpinBox();

    m_rotationX->setSingleStep(s_angleStep);
    m_rotationY->setSingleStep(s_angleStep);
    m_rotationZ->setSingleStep(s_angleStep);

    m_rotationX->setRange(s_angleMin, s_angleMax);
    m_rotationY->setRange(s_angleMin, s_angleMax);
    m_rotationZ->setRange(s_angleMin, s_angleMax);

    m_rotationX->setWrapping(true);
    m_rotationY->setWrapping(true);
    m_rotationZ->setWrapping(true);

    m_rotationX->setValue(m_transform->getRotation().x * 180 / std::numbers::pi);
    m_rotationY->setValue(m_transform->getRotation().y * 180 / std::numbers::pi);
    m_rotationZ->setValue(m_transform->getRotation().z * 180 / std::numbers::pi);

    m_rotationX->setKeyboardTracking(true);
    m_rotationY->setKeyboardTracking(true);
    m_rotationZ->setKeyboardTracking(true);

    m_rotationX->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_rotationY->setFixedWidth(s_doubleSpinBoxFixedWidth);
    m_rotationZ->setFixedWidth(s_doubleSpinBoxFixedWidth);

    connect(m_rotationX, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onRotationXChanged);
    connect(m_rotationY, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onRotationYChanged);
    connect(m_rotationZ, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onRotationZChanged);
    const auto rotationLayout = new QHBoxLayout();
    rotationLayout->addWidget(m_rotationX);
    rotationLayout->addWidget(m_rotationY);
    rotationLayout->addWidget(m_rotationZ);
    layout->addRow(new QLabel("Rotation"), rotationLayout);
}
