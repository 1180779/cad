//
// Created by Radosław Głasek on 21.06.2026
//

#include "CadMenuBar.hpp"

#include <QAction>
#include <QDoubleSpinBox>
#include <QMenu>
#include <QWidget>
#include <QWidgetAction>

#include "WidgetBuilders.hxx"
#include "input/InputMap.hpp"

namespace {
    /// @brief Builds a labelled @c QDoubleSpinBox embedded in a menu via @c
    /// QWidgetAction
    QDoubleSpinBox* addSpinBoxAction(
        QMenu *menu,
        const QString &label,
        const double value,
        const double min,
        const double max,
        const double step
    ) {
        // ReSharper disable once CppDFAMemoryLeak
        auto *container = new QWidget(menu);
        // ReSharper disable once CppDFAMemoryLeak
        auto *layout = new QHBoxLayout(container);
        layout->setContentsMargins(8, 4, 8, 4);
        // ReSharper disable once CppDFAMemoryLeak
        const auto spin = widgets::newDoubleSpinBox(min, max, value);
        spin->setSingleStep(step);
        layout->addWidget(new QLabel(label, container));
        layout->addStretch();
        layout->addWidget(spin);

        // ReSharper disable once CppDFAMemoryLeak
        auto *action = new QWidgetAction(menu);
        action->setDefaultWidget(container);
        menu->addAction(action);
        return spin;
    }

    /// @brief Adds a checkable menu action
    QAction* addCheckableAction(QMenu *menu, const QString &text, const bool checked = false) {
        auto *action = menu->addAction(text);
        action->setCheckable(true);
        action->setChecked(checked);
        return action;
    }
}

CadMenuBar::CadMenuBar(QWidget *parent)
: QMenuBar(parent),
  m_fileMenu{addMenu("File")},
  m_newAction{m_fileMenu->addAction("New", this, &CadMenuBar::newRequested)},
  m_saveAction{m_fileMenu->addAction("Save", this, &CadMenuBar::saveRequested)},
  m_saveAsAction{
      m_fileMenu->addAction("Save as", this, &CadMenuBar::saveAsRequested)
  },
  m_openAction{
      m_fileMenu->addAction("Load", this, &CadMenuBar::openRequested)
  },
  m_editMenu{addMenu("Edit")},
  m_undoAction{m_editMenu->addAction("Undo", this, &CadMenuBar::undoRequested)},
  m_redoAction{
      m_editMenu->addAction("Redo", this, &CadMenuBar::redoRequested)
  },
  m_toolsMenu{addMenu("Tools")},
  m_viewMenu{addMenu("View")},
  m_stereoMenu{addMenu("Stereo")},

  m_stereoEnableToggle{
      addCheckableAction(m_stereoMenu, "Enable Stereoscopy")
  },
  m_stereoLuminanceModeToggle{
      addCheckableAction(m_stereoMenu, "Luminance anaglyph", true)
  },
  m_stereoAutoTrackToggle{
      addCheckableAction(m_stereoMenu, "Auto (track camera)", true)
  },
  m_stereoAutoEyeSepToggle{
      addCheckableAction(m_stereoMenu, "Auto eye distance", true)
  },
  m_stereoEyeSepSpinbox{
      addSpinBoxAction(m_stereoMenu, "Eye distance", 0.3, 0.0, 100.0, 0.05)
  },
  m_stereoConvergenceSpinbox{
      addSpinBoxAction(m_stereoMenu, "Plane distance", 10.0, 0.1, 1000.0, 0.5)
  },
  m_stereoSeparationRatioSpin{
      addSpinBoxAction(
          m_stereoMenu,
          "Depth divisor (1/N)",
          30.0,
          0.1,
          100.0,
          0.5
      )
  } {
    const std::array globalContextActions = {
        m_newAction,
        m_saveAction,
        m_saveAsAction,
        m_openAction,
        m_redoAction,
        m_undoAction
    };
    for (const auto action : globalContextActions) {
        action->setShortcutContext(Qt::ApplicationShortcut);
    }

    connect(m_editMenu, &QMenu::aboutToShow, this, &CadMenuBar::isAboutToShow);
    const auto *darkThemeToggle = addCheckableAction(m_viewMenu, "Dark theme");
    connect(darkThemeToggle, &QAction::toggled, this, &CadMenuBar::darkThemeChanged);
    connect(m_stereoEnableToggle, &QAction::toggled, this, &CadMenuBar::stereoEnabledChanged);
    connect(m_stereoAutoTrackToggle, &QAction::toggled, this, &CadMenuBar::stereoAutoChanged);
    connect(m_stereoAutoEyeSepToggle, &QAction::toggled, this, &CadMenuBar::stereoAutoEyeSepChanged);
    connect(m_stereoLuminanceModeToggle, &QAction::toggled, this, &CadMenuBar::stereoLuminanceChanged);
    connect(m_stereoEyeSepSpinbox, &QDoubleSpinBox::valueChanged, this, &CadMenuBar::stereoEyeSeparationChanged);
    connect(m_stereoConvergenceSpinbox, &QDoubleSpinBox::valueChanged, this, &CadMenuBar::stereoConvergenceChanged);
    connect(m_stereoSeparationRatioSpin, &QDoubleSpinBox::valueChanged, this, &CadMenuBar::stereoSepRatioChanged);
    connect(m_stereoAutoTrackToggle, &QAction::toggled, this, &CadMenuBar::updateSpinsEnabled);
    connect(m_stereoAutoEyeSepToggle, &QAction::toggled, this, &CadMenuBar::updateSpinsEnabled);
    updateSpinsEnabled(true);
}

void CadMenuBar::applyShortcuts(const InputMap &inputMap) const {
    const std::map<QAction*, InputAction> actionMap{
        {m_newAction, InputAction::newFile},
        {m_saveAction, InputAction::save},
        {m_saveAsAction, InputAction::saveAs},
        {m_openAction, InputAction::open},

        {m_undoAction, InputAction::undo},
        {m_redoAction, InputAction::redo}
    };

    for (const auto [qAction, inputAction] : actionMap) {
        qAction->setShortcuts(inputMap.sequencesFor(inputAction));
    }
}

void CadMenuBar::setUndoEnabled(const bool enabled) const {
    m_undoAction->setEnabled(enabled);
}

void CadMenuBar::setRedoEnabled(const bool enabled) const {
    m_redoAction->setEnabled(enabled);
}

void CadMenuBar::setFileActionsEnabled(const bool enabled) const {
    m_newAction->setEnabled(enabled);
    m_saveAction->setEnabled(enabled);
    m_saveAsAction->setEnabled(enabled);
    m_openAction->setEnabled(enabled);
}

void CadMenuBar::setStereoEyeSep(const double eyeSep) const {
    const QSignalBlocker block(m_stereoEyeSepSpinbox);
    m_stereoEyeSepSpinbox->setValue(eyeSep);
}

void CadMenuBar::setStereoConvergence(const double convergence) const {
    const QSignalBlocker block(m_stereoConvergenceSpinbox);
    m_stereoConvergenceSpinbox->setValue(convergence);
}

QAction* CadMenuBar::addToolPanelAction(const QString &name) const {
    return addCheckableAction(m_toolsMenu, name, true);
}

void CadMenuBar::isAboutToShow() {
    emit editMenuAboutToShow();
}

void CadMenuBar::updateSpinsEnabled(bool) const {
    const bool autoOn = m_stereoAutoTrackToggle->isChecked();
    m_stereoConvergenceSpinbox->setEnabled(!autoOn);
    m_stereoEyeSepSpinbox->setEnabled(!(autoOn && m_stereoAutoEyeSepToggle->isChecked()));
}
