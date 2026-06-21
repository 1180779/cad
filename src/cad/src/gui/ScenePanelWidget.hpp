//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_SCENEPANELWIDGET_HPP
#define CAD_SCENEPANELWIDGET_HPP

#include "EntityPropertiesWidget.hpp"
#include "SceneHierarchyWidget.hpp"
#include "ToolPanelWidget.hpp"

class ScenePanelWidget final : public ToolPanelWidget {
    Q_OBJECT

public:
    explicit ScenePanelWidget(QWidget *parent = nullptr);

    [[nodiscard]] SceneHierarchyWidget* hierarchyWidget() const;

    [[nodiscard]] EntityPropertiesWidget* entityPropertiesWidget() const;

private:
    SceneHierarchyWidget *m_hierarchy;
    EntityPropertiesWidget *m_entityProperties;
};

#endif //CAD_SCENEPANELWIDGET_HPP
