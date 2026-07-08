//
// Created by Radosław Głasek on 07.07.2026
//

#ifndef CAD_PROPERTIESPANELWIDGET_HXX
#define CAD_PROPERTIESPANELWIDGET_HXX

#include "EntityPropertiesWidget.hpp"
#include "../ToolPanelWidget.hpp"

/// @brief Dockable tool window for @c EntityPropertiesWidget
class PropertiesPanelWidget final : public ToolPanelWidget {
    Q_OBJECT

public:
    explicit PropertiesPanelWidget(QWidget *parent = nullptr);

    [[nodiscard]] EntityPropertiesWidget* entityPropertiesWidget() const;

private:
    EntityPropertiesWidget *m_entityProperties;
};

#endif //CAD_PROPERTIESPANELWIDGET_HXX
