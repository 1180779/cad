//
// Created by Radosław Głasek on 07.07.2026
//

#include "PropertiesPanelWidget.hxx"

#include <QVBoxLayout>

PropertiesPanelWidget::PropertiesPanelWidget(QWidget *parent)
: ToolPanelWidget("Properties", parent),
  m_entityProperties(new EntityPropertiesWidget) {
    createScrollLayout(m_entityProperties);
}

EntityPropertiesWidget* PropertiesPanelWidget::entityPropertiesWidget() const {
    return m_entityProperties;
}
