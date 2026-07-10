//
// Created by Radosław Głasek on 21.06.2026
//

#include "ViewportPanelWidget.hpp"

#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

#include "GridSettingsWidget.hpp"
#include "ViewportTypes.hpp"

// ReSharper disable CppDFAMemoryLeak

ViewportPanelWidget::ViewportPanelWidget(QWidget *parent)
: ToolPanelWidget("Viewport", parent),
  m_gridSettings(new GridSettingsWidget(this)),
  m_alignCameraWidget{new AlignCameraToPlaneWidget(this)},
  m_pivotCombo(new QComboBox(this)),
  m_coordSpaceCombo(new QComboBox(this)) {
    m_pivotCombo->addItem("Median point", static_cast<int>(PivotMode::medianPoint));
    m_pivotCombo->addItem("Active cursor", static_cast<int>(PivotMode::activeCursor));

    m_coordSpaceCombo->addItem("World", static_cast<int>(CoordSpace::world));
    m_coordSpaceCombo->addItem("Local", static_cast<int>(CoordSpace::local));

    const auto content = new QWidget;
    const auto contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setAlignment(Qt::AlignTop);
    contentLayout->addWidget(m_gridSettings);
    contentLayout->addWidget(new QLabel("Transform pivot:", this));
    contentLayout->addWidget(m_pivotCombo);
    contentLayout->addWidget(new QLabel("Transform space:", this));
    contentLayout->addWidget(m_coordSpaceCombo);
    contentLayout->addWidget(m_alignCameraWidget);

    createScrollLayout(content);
}

// ReSharper restore CppDFAMemoryLeak

const GridSettingsWidget* ViewportPanelWidget::gridSettingsWidget() const {
    return m_gridSettings;
}

const QComboBox* ViewportPanelWidget::pivotCombo() const {
    return m_pivotCombo;
}

const QComboBox* ViewportPanelWidget::coordSpaceCombo() const {
    return m_coordSpaceCombo;
}

const AlignCameraToPlaneWidget* ViewportPanelWidget::alignCameraWidget() const {
    return m_alignCameraWidget;
}
