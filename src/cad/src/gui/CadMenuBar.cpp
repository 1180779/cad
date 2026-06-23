//
// Created by Radosław Głasek on 21.06.2026
//

#include "CadMenuBar.hpp"

#include <QAction>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QMenu>
#include <QWidget>
#include <QWidgetAction>

#include "input/InputMap.hpp"

namespace {
    /// @brief Builds a labelled QDoubleSpinBox embedded in a menu via QWidgetAction
    QDoubleSpinBox* addSpinBoxAction(
        QMenu *menu,
        const QString &label,
        const double value,
        const double min,
        const double max,
        const double step
    ) {
        auto *container = new QWidget(menu);
        auto *layout = new QFormLayout(container);
        layout->setContentsMargins(8, 4, 8, 4);
        auto *spin = new QDoubleSpinBox(container);
        spin->setRange(min, max);
        spin->setSingleStep(step);
        spin->setValue(value);
        layout->addRow(label, spin);

        auto *action = new QWidgetAction(menu);
        action->setDefaultWidget(container);
        menu->addAction(action);
        return spin;
    }
}

CadMenuBar::CadMenuBar(QWidget *parent) : QMenuBar(parent) {
    m_editMenu = addMenu("Edit");
    m_undoAction = m_editMenu->addAction("Undo", this, &CadMenuBar::undoRequested);
    m_redoAction = m_editMenu->addAction("Redo", this, &CadMenuBar::redoRequested);
    m_undoAction->setShortcutContext(Qt::ApplicationShortcut);
    m_redoAction->setShortcutContext(Qt::ApplicationShortcut);

    connect(
        m_editMenu,
        &QMenu::aboutToShow,
        this,
        [this] {
            emit editMenuAboutToShow();
        }
    );

    m_toolsMenu = addMenu("Tools");

    m_viewMenu = addMenu("View");
    auto *darkThemeToggle = m_viewMenu->addAction("Dark theme");
    darkThemeToggle->setCheckable(true);
    connect(darkThemeToggle, &QAction::toggled, this, &CadMenuBar::darkThemeChanged);

    m_stereoMenu = addMenu("Stereo");
    auto *stereoToggle = m_stereoMenu->addAction("Enable Stereoscopy");
    stereoToggle->setCheckable(true);
    connect(stereoToggle, &QAction::toggled, this, &CadMenuBar::stereoEnabledChanged);

    m_stereoMenu->addSeparator();
    auto *eyeSepSpin = addSpinBoxAction(m_stereoMenu, "Eye distance", 0.3, 0.0, 100.0, 0.05);
    connect(eyeSepSpin, &QDoubleSpinBox::valueChanged, this, &CadMenuBar::stereoEyeSeparationChanged);
    auto *convergenceSpin = addSpinBoxAction(m_stereoMenu, "Plane distance", 10.0, 0.1, 1000.0, 0.5);
    connect(convergenceSpin, &QDoubleSpinBox::valueChanged, this, &CadMenuBar::stereoConvergenceChanged);
}

void CadMenuBar::applyShortcuts(const InputMap &inputMap) const {
    m_undoAction->setShortcuts(inputMap.sequencesFor(InputAction::undo));
    m_redoAction->setShortcuts(inputMap.sequencesFor(InputAction::redo));
}

void CadMenuBar::setUndoEnabled(const bool enabled) const {
    m_undoAction->setEnabled(enabled);
}

void CadMenuBar::setRedoEnabled(const bool enabled) const {
    m_redoAction->setEnabled(enabled);
}

QAction* CadMenuBar::addToolPanelAction(const QString &name) const {
    auto *action = m_toolsMenu->addAction(name);
    action->setCheckable(true);
    action->setChecked(true);
    return action;
}
