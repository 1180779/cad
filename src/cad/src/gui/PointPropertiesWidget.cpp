//
// Created on 3/31/26.
//

#include "PointPropertiesWidget.hpp"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

#include "../Scene.hpp"
#include "../commands/CommandStack.hpp"
#include "../commands/Commands.hpp"

PointPropertiesWidget::PointPropertiesWidget(PointRegistry *registry, QWidget *parent) : QWidget(parent),
    m_registry{registry} {
    const auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    const auto setupSb = [&](const QString &label, ModifierDoubleSpinBox *&sb) {
        layout->addWidget(new QLabel(label));
        sb = new ModifierDoubleSpinBox();
        sb->setRange(s_coordMin, s_coordMax);
        sb->setSingleStep(s_coordStep);
        sb->setDecimals(4);
        sb->setFixedWidth(s_widgetWidth);
        sb->setKeyboardTracking(true);
        sb->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        layout->addWidget(sb);
    };

    setupSb("X:", m_x);
    setupSb("Y:", m_y);
    setupSb("Z:", m_z);

    connect(m_x, &QDoubleSpinBox::valueChanged, this, &PointPropertiesWidget::onXChanged);
    connect(m_y, &QDoubleSpinBox::valueChanged, this, &PointPropertiesWidget::onYChanged);
    connect(m_z, &QDoubleSpinBox::valueChanged, this, &PointPropertiesWidget::onZChanged);

    setEnabled(false);
}

void PointPropertiesWidget::refresh() {
    setPoint(m_handle);
}

void PointPropertiesWidget::setPoint(const PointHandle handle) {
    m_handle = handle;

    const bool valid = m_registry != nullptr && handle != InvalidPointHandle && m_registry->isAlive(handle);
    setEnabled(valid);
    // setVisible(valid);

    if (!valid) {
        return;
    }

    const auto pos = m_registry->getPosition(handle);

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

void PointPropertiesWidget::applyCoordEdit(const int axis, const double value) {
    if (!m_registry || m_handle == InvalidPointHandle) {
        return;
    }
    const auto before = m_registry->getPosition(m_handle);
    auto after = before;
    after[axis] = static_cast<cadm::cadf>(value);

    if (m_scene && m_commandStack) {
        m_commandStack->push(std::make_unique<MovePointCommand>(*m_scene, m_handle, before, after), true);
    }
    else {
        m_registry->setPosition(m_handle, after);
    }
    emit propertyChanged();
}

void PointPropertiesWidget::onXChanged(const double value) {
    applyCoordEdit(cadm::Vec3::Index::X, value);
}

void PointPropertiesWidget::onYChanged(const double value) {
    applyCoordEdit(cadm::Vec3::Index::Y, value);
}

void PointPropertiesWidget::onZChanged(const double value) {
    applyCoordEdit(cadm::Vec3::Index::Z, value);
}
