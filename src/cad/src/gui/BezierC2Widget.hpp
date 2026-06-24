//
// Created on 5/5/26.
//

#ifndef CAD_BEZIERC2WIDGET_HPP
#define CAD_BEZIERC2WIDGET_HPP

#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>
#include <unordered_map>

#include "ComponentWidget.hpp"
#include "VirtualPointPropertiesWidget.hpp"
#include "../components/BezierC2Component.hpp"
#include "../Scene.hpp"

/// @brief Properties panel for a C2 Bezier (B-spline) curve.
///
/// @details 
/// Shows two parallel point lists -- the editable de Boor control points and the
/// derived Bernstein points -- with shared X/Y/Z spinboxes that edit whichever of
/// the two is currently selected. 
/// 
/// Editing a de Boor point is a plain point move;
/// editing a Bernstein point back-computes the affected de Boor points. 
///
/// @note All edits go through the command stack (see ComponentWidget::pushEdit), so they
/// undo/redo and coalesce per gesture
class BezierC2Widget final : public ComponentWidget {
    Q_OBJECT

public:
    explicit BezierC2Widget(BezierC2Component *bezier, Scene *scene, QWidget *parent = nullptr);

    /// @brief Rebuild the de Boor list from scratch (clear + re-add every row)
    /// @note Used for the initial fill; prefer reconcileDeBoorList() for live updates
    void refreshList();

    /// @brief Update the de Boor list in place to match the curve's current points:
    /// reuse existing rows by handle, drop removed ones, insert new ones at their
    /// position and re-stamp the [i] prefix on shifted rows. Preserves selection/scroll
    void reconcileDeBoorList();

    /// @brief Mirror the scene's entity selection onto the de Boor list rows
    void syncSelection();

    /// @brief Re-sync the panel with the curve after a structural change:
    /// reconciles the lists (de Boor; Bernstein) and refreshes the
    /// spinbox values for the currently selected point
    void refresh();

    /// @brief Re-sync the panel after a position-only change: 
    /// refreshes just the selected point's spinboxes, leaving the lists untouched
    void refreshGeometry() const;

signals :
    /// @brief Emitted when the de Boor list selection changes, carrying the point
    /// entities so the rest of the app can sync viewport/hierarchy selection
    void pointSelectionChanged(QList<Entity*> selected);

private:
    /// @brief Which of the two lists uses the shared spinboxes right now
    enum class SelectedPointKind { none, deBoor, bernstein };

    /// @brief Rebuild the Bernstein list from scratch (clear + re-add every row)
    /// @note Used for the initial fill / curve (re)loading; prefer reconciling
    /// for live updates so editing a point doesn't drop rows or cause flicker
    void refreshBernsteinList() const;

    /// @brief Update the Bernstein list in place to match the curve's Bernstein point
    /// count: append/remove only the trailing rows. Row labels derive purely from the
    /// index, so existing rows are left untouched -- preserving selection and the edit
    void reconcileBernsteinList() const;

    /// @brief Refresh the shared editor from whichever point (de Boor / Bernstein) is
    /// currently selected, without touching the lists
    void refreshSelectedSpinboxes() const;

    /// @brief Load the editor with the position of de Boor point @p h
    void updateSpinboxesForDeBoor(PointHandle h) const;

    /// @brief Load the editor with the Bernstein position at @p index
    void updateSpinboxesForBernstein(int index) const;

    /// @brief Commit a coordinate edit from the shared editor to whichever point
    /// (de Boor or Bernstein) is currently selected
    void onCoordinateEdited(cadm::Vec3 newPos);

    /// @brief Remove the selected de Boor point from the curve
    void onRemoveClicked();

    /// @brief Handle de Boor list selection
    void onDeBoorSelectionChanged();

    /// @brief Handle Bernstein list selection
    void onBernsteinSelectionChanged();

    BezierC2Component *m_bezier;
    Scene *m_scene;

    QCheckBox *m_showDeBoorPolygonCheckbox{};
    QCheckBox *m_showBernsteinPolygonCheckbox{};
    QCheckBox *m_showBernsteinCpsCheckbox{};

    /// @brief Editable control points (curve order)
    QListWidget *m_deBoorList{};

    /// @brief Removes the selected de Boor point from the curve
    QPushButton *m_removeButton{};

    /// @brief Derived Bernstein points (read-only set)
    QListWidget *m_bernsteinList{};

    /// @brief Shared X/Y/Z editor for the currently selected de Boor / Bernstein point
    VirtualPointPropertiesWidget *m_pointProps{};

    /// @brief Handle -> de Boor row, so reconcileDeBoorList can reuse rows by handle
    std::unordered_map<PointHandle, QListWidgetItem*> m_deBoorItemMap;

    SelectedPointKind m_selectedKind = SelectedPointKind::none;

    /// @note valid when m_selectedKind == deBoor
    PointHandle m_selectedDeBoorHandle = InvalidPointHandle;

    /// @brief Bernstein index
    /// @note valid when m_selectedKind == bernstein
    int m_selectedBernsteinIndex = -1;
};

#endif //CAD_BEZIERC2WIDGET_HPP
