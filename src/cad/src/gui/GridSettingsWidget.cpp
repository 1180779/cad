//
// Created on 3/25/26.
//

#include "GridSettingsWidget.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>

GridSettingsWidget::GridSettingsWidget(QWidget *parent) : QWidget(parent) {
    // ReSharper disable once CppDFAMemoryLeak
    const auto outerLayout = new QVBoxLayout(this);
    outerLayout->setAlignment(Qt::AlignTop);

    // ReSharper disable once CppDFAMemoryLeak
    const auto groupBox = new QGroupBox("Grid Planes");
    // ReSharper disable once CppDFAMemoryLeak
    const auto groupLayout = new QVBoxLayout(groupBox);

    m_xyPlane = new QCheckBox("XY Plane (z = 0)");
    m_xzPlane = new QCheckBox("XZ Plane (y = 0)");
    m_yzPlane = new QCheckBox("YZ Plane (x = 0)");

    m_xyPlane->setChecked(false);
    m_xzPlane->setChecked(true);
    m_yzPlane->setChecked(false);

    groupLayout->addWidget(m_xyPlane);
    groupLayout->addWidget(m_xzPlane);
    groupLayout->addWidget(m_yzPlane);

    outerLayout->addWidget(groupBox);

    connect(m_xyPlane, &QCheckBox::toggled, this, &GridSettingsWidget::onCheckboxToggled);
    connect(m_xzPlane, &QCheckBox::toggled, this, &GridSettingsWidget::onCheckboxToggled);
    connect(m_yzPlane, &QCheckBox::toggled, this, &GridSettingsWidget::onCheckboxToggled);

    // ReSharper disable once CppDFAMemoryLeak
    const auto axesBox = new QGroupBox("Axes");
    // ReSharper disable once CppDFAMemoryLeak
    const auto axesLayout = new QVBoxLayout(axesBox);

    m_xAxis = new QCheckBox("X");
    m_yAxis = new QCheckBox("Y");
    m_zAxis = new QCheckBox("Z");
    m_xAxis->setChecked(true);
    m_yAxis->setChecked(true);
    m_zAxis->setChecked(true);

    axesLayout->addWidget(m_xAxis);
    axesLayout->addWidget(m_yAxis);
    axesLayout->addWidget(m_zAxis);
    outerLayout->addWidget(axesBox);

    const auto emitAxes = [this] {
        emit axesMaskChanged(getAxesMask());
    };
    connect(m_xAxis, &QCheckBox::toggled, this, emitAxes);
    connect(m_yAxis, &QCheckBox::toggled, this, emitAxes);
    connect(m_zAxis, &QCheckBox::toggled, this, emitAxes);
}

int GridSettingsWidget::getAxesMask() const {
    int mask = 0;
    if (m_xAxis->isChecked()) {
        mask |= 1 << 0;
    }
    if (m_yAxis->isChecked()) {
        mask |= 1 << 1;
    }
    if (m_zAxis->isChecked()) {
        mask |= 1 << 2;
    }
    return mask;
}

int GridSettingsWidget::getGridPlanes() const {
    int planes = 0;
    if (m_xyPlane->isChecked()) {
        planes |= 1 << 0;
    }
    if (m_xzPlane->isChecked()) {
        planes |= 1 << 1;
    }
    if (m_yzPlane->isChecked()) {
        planes |= 1 << 2;
    }
    return planes;
}

void GridSettingsWidget::onCheckboxToggled() {
    emit gridPlanesChanged(getGridPlanes());
}
