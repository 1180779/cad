//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_CADMENUBAR_HPP
#define CAD_CADMENUBAR_HPP

#include <functional>

#include <QDoubleSpinBox>
#include <QMenuBar>
#include <QActionGroup>

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

    /// @brief Enables/disables File actions, including their keyboard
    /// shortcuts. Used to block them while a modal-ish dialogs are open, (when
    /// overlay only blocks mouse clicks on parts of the ui but not
    /// application-wide shortcuts)
    void setFileActionsEnabled(bool enabled) const;

    void setStereoEyeSep(double eyeSep) const;

    void setStereoConvergence(double convergence) const;

    void addCursorStrategy(const QString &name, const std::function<void()> &applyStrategy);

    void synchronizeSelectedCursorStrategy(std::size_t index) const;

    /// @brief Register a toggleable panel entry under the Tools menu; returns the action
    [[nodiscard]] QAction* addToolPanelAction(const QString &name) const;

signals:
    void newRequested();

    void saveRequested();

    void saveAsRequested();

    void openRequested();

    void undoRequested();

    void redoRequested();

    void darkThemeChanged(bool enabled);

    void stereoEnabledChanged(bool enabled);

    void stereoAutoChanged(bool enabled);

    void stereoLuminanceChanged(bool enabled);

    void stereoAutoEyeSepChanged(bool enabled);

    void stereoEyeSeparationChanged(double sep);

    void stereoConvergenceChanged(double dist);

    void stereoSepRatioChanged(double ratio);

    /// @brief Emitted just before the Edit menu opens so callers can refresh
    /// enabled state
    void editMenuAboutToShow();

private:
    void isAboutToShow();

    void updateSpinsEnabled(bool) const;

    QMenu *const m_fileMenu;
    QAction *const m_newAction;
    QAction *const m_saveAction;
    QAction *const m_saveAsAction;
    QAction *const m_openAction;

    QMenu *const m_editMenu;
    QAction *const m_undoAction;
    QAction *const m_redoAction;

    QMenu *const m_toolsMenu;
    QMenu *const m_viewMenu;

    QMenu *const m_stereoMenu;
    QAction *const m_stereoEnableToggle;
    QAction *const m_stereoLuminanceModeToggle;
    QAction *const m_stereoAutoTrackToggle;
    QAction *const m_stereoAutoEyeSepToggle;
    QDoubleSpinBox *const m_stereoEyeSepSpinbox;
    QDoubleSpinBox *const m_stereoConvergenceSpinbox;
    QDoubleSpinBox *const m_stereoSeparationRatioSpin;

    QMenu *const m_cursorStrategyMenu;
    QActionGroup *const m_cursorStrategiesGroup;
};

#endif //CAD_CADMENUBAR_HPP
