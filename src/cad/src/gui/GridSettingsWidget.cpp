//
// Created on 3/25/26.
//

#include "GridSettingsWidget.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>

GridSettingsWidget::GridSettingsWidget(QWidget *parent) : QWidget(parent) {
    const auto outerLayout = new QVBoxLayout(this);
    outerLayout->setAlignment(Qt::AlignTop);

    const auto groupBox = new QGroupBox("Grid Planes");
    const auto groupLayout = new QVBoxLayout(groupBox);

    m_xyPlane = new QCheckBox("XY Plane  (z = 0)");
    m_xzPlane = new QCheckBox("XZ Plane  (y = 0)");
    m_yzPlane = new QCheckBox("YZ Plane  (x = 0)");

    m_xyPlane->setChecked(true);
    m_xzPlane->setChecked(false);
    m_yzPlane->setChecked(false);

    groupLayout->addWidget(m_xyPlane);
    groupLayout->addWidget(m_xzPlane);
    groupLayout->addWidget(m_yzPlane);

    outerLayout->addWidget(groupBox);

    connect(m_xyPlane, &QCheckBox::toggled, this, &GridSettingsWidget::onCheckboxToggled);
    connect(m_xzPlane, &QCheckBox::toggled, this, &GridSettingsWidget::onCheckboxToggled);
    connect(m_yzPlane, &QCheckBox::toggled, this, &GridSettingsWidget::onCheckboxToggled);
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
