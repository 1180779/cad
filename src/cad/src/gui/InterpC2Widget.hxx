//
// Created by Radosław Głasek on 23.06.2026
//

#ifndef CAD_INTERPC2WIDGET_HXX
#define CAD_INTERPC2WIDGET_HXX

#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <unordered_map>

#include "ComponentWidget.hpp"
#include "VirtualPointPropertiesWidget.hpp"
#include "../components/InterpC2Component.hxx"
#include "../Scene.hpp"

/// @brief Properties panel for an interpolating C2 spline
class InterpC2Widget final : public ComponentWidget {
    Q_OBJECT

public:
    explicit InterpC2Widget(InterpC2Component *curve, Scene *scene, QWidget *parent = nullptr);

    /// @brief Rebuild the point list from scratch
    void refreshList();

    /// @brief Update the point list in place to match the curve's current points
    void reconcileList();

    /// @brief Mirror the scene's entity selection onto the list rows
    void syncSelection();

    /// @brief Re-sync the panel after a structural change
    void refresh();

    /// @brief Re-sync the selected point's spinboxes after a position-only change
    void refreshGeometry() const;

signals :
    void pointSelectionChanged(QList<Entity*> selected);

private:
    void onRemoveClicked();

    void onListSelectionChanged();

    void onCoordinateEdited(cadm::Vec3 newPos);

    void updateSpinboxes(PointHandle h) const;

    InterpC2Component *m_curve;
    Scene *m_scene;

    QCheckBox *m_showPolylineCheckbox{};
    QCheckBox *m_showBernsteinPolygonCheckbox{};
    QCheckBox *m_showBernsteinCpsCheckbox{};

    QListWidget *m_pointList{};
    QPushButton *m_removeButton{};
    VirtualPointPropertiesWidget *m_pointProps{};

    std::unordered_map<PointHandle, QListWidgetItem*> m_itemMap;

    PointHandle m_selectedHandle = InvalidPointHandle;
};

#endif //CAD_INTERPC2WIDGET_HXX
