//
// Created on 3/25/26.
//

#include "GridSettingsWidget.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>

// ReSharper disable CppDFAMemoryLeak

GridSettingsWidget::GridSettingsWidget(QWidget *parent)
: QWidget(parent) {
    const auto outerLayout = new QVBoxLayout(this);
    outerLayout->setAlignment(Qt::AlignTop);

    const auto groupBox = new QGroupBox("Grid Planes");
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

    const auto axesBox = new QGroupBox("Axes");
    const auto axesLayout = new QVBoxLayout(axesBox);
    const auto addAxisCheckBox = [&](const QString &text) {
        const auto axis = new QCheckBox(text);
        axis->setChecked(true);
        axesLayout->addWidget(axis);
        return axis;
    };

    m_xAxis = addAxisCheckBox("X");
    m_yAxis = addAxisCheckBox("Y");
    m_zAxis = addAxisCheckBox("Z");
    outerLayout->addWidget(axesBox);

    const auto emitAxes = [this] {
        emit axesMaskChanged(getAxesMask());
    };
    connect(m_xAxis, &QCheckBox::toggled, this, emitAxes);
    connect(m_yAxis, &QCheckBox::toggled, this, emitAxes);
    connect(m_zAxis, &QCheckBox::toggled, this, emitAxes);

    m_lodFade = new QCheckBox("Fade distant grid / axes");
    m_lodFade->setChecked(false);
    outerLayout->addWidget(m_lodFade);
    connect(
        m_lodFade,
        &QCheckBox::toggled,
        this,
        [this](const bool checked) {
            emit lodFadeChanged(checked);
        }
    );
}

// ReSharper restore CppDFAMemoryLeak

bool GridSettingsWidget::getLodFade() const {
    return m_lodFade->isChecked();
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
