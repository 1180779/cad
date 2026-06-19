//
// Created on 5/5/26.
//

#ifndef CAD_BEZIERC2WIDGET_HPP
#define CAD_BEZIERC2WIDGET_HPP

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>
#include <unordered_map>

#include "ComponentWidget.hpp"
#include "../components/BezierC2Component.hpp"
#include "../Scene.hpp"

class BezierC2Widget final : public ComponentWidget {
    Q_OBJECT

public:
    explicit BezierC2Widget(BezierC2Component *bezier, Scene *scene, QWidget *parent = nullptr);

    void refreshList();

    void syncSelection();

    /// Should be called on geometry change.
    /// Only refreshes spinbox values for the currently selected point.
    /// 
    /// TODO: actually do what the description says (not done yet)
    void refresh();

signals :
    void pointSelectionChanged(QList<Entity*> selected);

private:
    enum class SelectedPointKind { none, deBoor, bernstein };

    void refreshBernsteinList() const;

    void updateSpinboxesForDeBoor(PointHandle h);

    void updateSpinboxesForBernstein(int index);

    void setSpinboxesEnabled(bool enabled) const;

    BezierC2Component *m_bezier;
    Scene *m_scene;

    QCheckBox *m_showDeBoorPolygonCheckbox{};
    QCheckBox *m_showBernsteinPolygonCheckbox{};
    QCheckBox *m_uniformCheckbox{};

    QListWidget *m_deBoorList{};
    QPushButton *m_removeButton{};

    QListWidget *m_bernsteinList{};

    QLabel *m_selectedLabel{};
    QDoubleSpinBox *m_xSpin{};
    QDoubleSpinBox *m_ySpin{};
    QDoubleSpinBox *m_zSpin{};

    std::unordered_map<PointHandle, QListWidgetItem*> m_deBoorItemMap;

    SelectedPointKind m_selectedKind = SelectedPointKind::none;
    PointHandle m_selectedDeBoor = InvalidPointHandle;
    int m_selectedBernstein = -1;

    bool m_spinboxRefreshing = false;
};

#endif //CAD_BEZIERC2WIDGET_HPP
