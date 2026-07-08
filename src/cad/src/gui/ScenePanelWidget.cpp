//
// Created by Radosław Głasek on 21.06.2026
//

#include "ScenePanelWidget.hpp"

#include <QVBoxLayout>

#include "SceneHierarchyWidget.hpp"

ScenePanelWidget::ScenePanelWidget(QWidget *parent)
: ToolPanelWidget("Scene", parent),
  m_hierarchy(new SceneHierarchyWidget(this)) {
    const auto layout = createLayout();
    layout->addWidget(m_hierarchy);
}

SceneHierarchyWidget* ScenePanelWidget::hierarchyWidget() const {
    return m_hierarchy;
}
