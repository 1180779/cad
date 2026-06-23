//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_CADMENUBAR_HPP
#define CAD_CADMENUBAR_HPP

#include <QMenuBar>

#include "input/InputMap.hpp"

class CadMenuBar final : public QMenuBar {
    Q_OBJECT

public:
    explicit CadMenuBar(QWidget *parent = nullptr);

    /// @brief Sources the Undo/Redo menu shortcuts from the input map.
    /// Keeps the binding table the single source of truth and lets the menu
    /// actions own (and display) the chords
    void applyShortcuts(const InputMap &inputMap) const;

    void setUndoEnabled(bool enabled) const;

    void setRedoEnabled(bool enabled) const;

    /// @brief Register a toggleable panel entry under the Tools menu; returns the action
    [[nodiscard]] QAction* addToolPanelAction(const QString &name) const;

signals:
    void undoRequested();

    void redoRequested();

    void darkThemeChanged(bool enabled);

    void stereoEnabledChanged(bool enabled);

    void stereoEyeSeparationChanged(double sep);

    void stereoConvergenceChanged(double dist);

    /// @brief Emitted just before the Edit menu opens so callers can refresh enabled state
    void editMenuAboutToShow();

private:
    QMenu *m_editMenu;
    QMenu *m_toolsMenu;
    QMenu *m_viewMenu;
    QMenu *m_stereoMenu;
    QAction *m_undoAction;
    QAction *m_redoAction;
};

#endif //CAD_CADMENUBAR_HPP
