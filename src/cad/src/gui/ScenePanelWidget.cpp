#include "ScenePanelWidget.hpp"

#include <QVBoxLayout>

#include "EntityPropertiesWidget.hpp"
#include "SceneHierarchyWidget.hpp"

ScenePanelWidget::ScenePanelWidget(QWidget *parent) : ToolPanelWidget("Scene", parent),
                                                      m_hierarchy(new SceneHierarchyWidget(this)),
                                                      m_entityProperties(new EntityPropertiesWidget(this)) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignTop);
    layout->addWidget(m_hierarchy);
    layout->addWidget(m_entityProperties);
}

SceneHierarchyWidget* ScenePanelWidget::hierarchyWidget() const {
    return m_hierarchy;
}

EntityPropertiesWidget* ScenePanelWidget::entityPropertiesWidget() const {
    return m_entityProperties;
}
