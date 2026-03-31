//
// Created on 3/31/26.
//

#include "PointPropertiesWidget.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>

PointPropertiesWidget::PointPropertiesWidget(QWidget *parent)
    : QWidget(parent)
{
    const auto form = new QFormLayout(this);

    m_x = new ModifierDoubleSpinBox();
    m_y = new ModifierDoubleSpinBox();
    m_z = new ModifierDoubleSpinBox();

    for (auto *sb : {m_x, m_y, m_z})
    {
        sb->setRange(s_coordMin, s_coordMax);
        sb->setSingleStep(s_coordStep);
        sb->setDecimals(4);
        sb->setFixedWidth(s_spinWidth);
        sb->setKeyboardTracking(true);
    }

    const auto row = new QHBoxLayout();
    row->addWidget(m_x);
    row->addWidget(m_y);
    row->addWidget(m_z);
    form->addRow(new QLabel("Position"), row);

    connect(m_x, &QDoubleSpinBox::valueChanged, this, &PointPropertiesWidget::onXChanged);
    connect(m_y, &QDoubleSpinBox::valueChanged, this, &PointPropertiesWidget::onYChanged);
    connect(m_z, &QDoubleSpinBox::valueChanged, this, &PointPropertiesWidget::onZChanged);

    QWidget::setVisible(false);
}

void PointPropertiesWidget::setPoint(PointRegistry *registry, const PointHandle handle)
{
    m_registry = registry;
    m_handle = handle;

    const bool valid = registry && handle != InvalidPointHandle && registry->isAlive(handle);
    setVisible(valid);

    if (!valid)
        return;

    const auto pos = registry->getPosition(handle);

    m_x->blockSignals(true);
    m_y->blockSignals(true);
    m_z->blockSignals(true);

    m_x->setValue(pos.x);
    m_y->setValue(pos.y);
    m_z->setValue(pos.z);

    m_x->blockSignals(false);
    m_y->blockSignals(false);
    m_z->blockSignals(false);
}

void PointPropertiesWidget::onXChanged(const double value)
{
    if (!m_registry || m_handle == InvalidPointHandle) return;
    auto pos = m_registry->getPosition(m_handle);
    pos.x = static_cast<cadm::cadf>(value);
    m_registry->setPosition(m_handle, pos);
    emit propertyChanged();
}

void PointPropertiesWidget::onYChanged(const double value)
{
    if (!m_registry || m_handle == InvalidPointHandle) return;
    auto pos = m_registry->getPosition(m_handle);
    pos.y = static_cast<cadm::cadf>(value);
    m_registry->setPosition(m_handle, pos);
    emit propertyChanged();
}

void PointPropertiesWidget::onZChanged(const double value)
{
    if (!m_registry || m_handle == InvalidPointHandle) return;
    auto pos = m_registry->getPosition(m_handle);
    pos.z = static_cast<cadm::cadf>(value);
    m_registry->setPosition(m_handle, pos);
    emit propertyChanged();
}
