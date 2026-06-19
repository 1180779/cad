//
// Created on 4/18/26.
//

#ifndef CAD_BEZIERC0WIDGET_HPP
#define CAD_BEZIERC0WIDGET_HPP

#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>
#include <unordered_map>

#include "ComponentWidget.hpp"
#include "PointPropertiesWidget.hpp"
#include "../components/BezierC0Component.hpp"
#include "../Scene.hpp"

class BezierC0Widget final : public ComponentWidget {
    Q_OBJECT

public:
    explicit BezierC0Widget(BezierC0Component *bezier, Scene *scene, QWidget *parent = nullptr);

    void setCommandContext(Scene *scene, CommandStack *stack, const EntityId id) override {
        ComponentWidget::setCommandContext(scene, stack, id);
        if (m_pointPropertiesWidget) {
            m_pointPropertiesWidget->setCommandContext(scene, stack);
        }
    }

    /// @brief Refresh the list items in the m_pointList list based on points from the m_bezier
    void populatePointList();

    /// @brief Sync the selection of list items in the m_pointList from the m_scene entities
    void syncSelectionFromScene();

    signals  :

    

    void pointSelectionChanged(QList<Entity*> selected);

private:
    /// @brief Associated bezier component
    BezierC0Component *m_bezier;

    /// @brief Source scene
    Scene *m_scene;

    QCheckBox *m_showPolygonCheckbox{};
    QListWidget *m_pointList{};

    /// @brief Map of points handles and items of the m_pointList items
    std::unordered_map<PointHandle, QListWidgetItem*> m_itemMap;

    /// @brief Button to detach a point handle from the curve
    QPushButton *m_detachButton{};

    /// @brief PointPropertiesWidget providing details for the currently selected point from the curve
    /// (provided exactly one is selected)
    PointPropertiesWidget *m_pointPropertiesWidget{};
};

#endif //CAD_BEZIERC0WIDGET_HPP
