//
// Created by Radosław Głasek on 21.06.2026
//

#include "ViewportPanelWidget.hpp"

#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

#include "GridSettingsWidget.hpp"
#include "ViewportTypes.hpp"

ViewportPanelWidget::ViewportPanelWidget(QWidget *parent) : ToolPanelWidget("Viewport", parent),
                                                            m_gridSettings(new GridSettingsWidget(this)),
                                                            m_pivotCombo(new QComboBox(this)),
                                                            m_coordSpaceCombo(new QComboBox(this)) {
    m_pivotCombo->addItem("Median point", static_cast<int>(PivotMode::medianPoint));
    m_pivotCombo->addItem("Active cursor", static_cast<int>(PivotMode::activeCursor));

    m_coordSpaceCombo->addItem("World", static_cast<int>(CoordSpace::world));
    m_coordSpaceCombo->addItem("Local", static_cast<int>(CoordSpace::local));

    // ReSharper disable once CppDFAMemoryLeak
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setAlignment(Qt::AlignTop);
    layout->addWidget(m_gridSettings);
    layout->addWidget(new QLabel("Transform pivot:", this));
    layout->addWidget(m_pivotCombo);
    layout->addWidget(new QLabel("Transform space:", this));
    layout->addWidget(m_coordSpaceCombo);
}

GridSettingsWidget* ViewportPanelWidget::gridSettingsWidget() const {
    return m_gridSettings;
}

QComboBox* ViewportPanelWidget::pivotCombo() const {
    return m_pivotCombo;
}

QComboBox* ViewportPanelWidget::coordSpaceCombo() const {
    return m_coordSpaceCombo;
}
