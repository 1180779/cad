//
// Created by Radosław Głasek on 21.06.2026
//

#include "CadMenuBar.hpp"

#include <QAction>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
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
        // ReSharper disable once CppDFAMemoryLeak
        auto *container = new QWidget(menu);
        // ReSharper disable once CppDFAMemoryLeak
        auto *layout = new QHBoxLayout(container);
        layout->setContentsMargins(8, 4, 8, 4);
        // ReSharper disable once CppDFAMemoryLeak
        auto *spin = new QDoubleSpinBox(container);
        spin->setRange(min, max);
        spin->setSingleStep(step);
        spin->setValue(value);
        layout->addWidget(new QLabel(label, container));
        layout->addStretch();
        layout->addWidget(spin);

        // ReSharper disable once CppDFAMemoryLeak
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

    auto *autoToggle = m_stereoMenu->addAction("Auto (track camera)");
    autoToggle->setCheckable(true);
    autoToggle->setChecked(true);
    connect(autoToggle, &QAction::toggled, this, &CadMenuBar::stereoAutoChanged);

    m_stereoEyeSepSpinbox = addSpinBoxAction(m_stereoMenu, "Eye distance", 0.3, 0.0, 100.0, 0.05);
    connect(m_stereoEyeSepSpinbox, &QDoubleSpinBox::valueChanged, this, &CadMenuBar::stereoEyeSeparationChanged);
    m_stereoConvergenceSpinbox = addSpinBoxAction(m_stereoMenu, "Plane distance", 10.0, 0.1, 1000.0, 0.5);
    connect(m_stereoConvergenceSpinbox, &QDoubleSpinBox::valueChanged, this, &CadMenuBar::stereoConvergenceChanged);
    const auto *stereoSeparationRatioSpin = addSpinBoxAction(
        m_stereoMenu,
        "Depth divisor (1/N)",
        30.0,
        0.1,
        100.0,
        0.5
    );
    connect(stereoSeparationRatioSpin, &QDoubleSpinBox::valueChanged, this, &CadMenuBar::stereoSepRatioChanged);

    connect(
        autoToggle,
        &QAction::toggled,
        this,
        [this](const bool on) {
            m_stereoEyeSepSpinbox->setEnabled(!on);
            m_stereoConvergenceSpinbox->setEnabled(!on);
        }
    );
    m_stereoEyeSepSpinbox->setEnabled(false);
    m_stereoConvergenceSpinbox->setEnabled(false);
    // ReSharper disable once CppDFAMemoryLeak
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

void CadMenuBar::setStereoEyeSep(const double eyeSep) const {
    const QSignalBlocker block(m_stereoEyeSepSpinbox);
    m_stereoEyeSepSpinbox->setValue(eyeSep);
}

void CadMenuBar::setStereoConvergence(const double convergence) const {
    const QSignalBlocker block(m_stereoConvergenceSpinbox);
    m_stereoConvergenceSpinbox->setValue(convergence);
}

QAction* CadMenuBar::addToolPanelAction(const QString &name) const {
    auto *action = m_toolsMenu->addAction(name);
    action->setCheckable(true);
    action->setChecked(true);
    return action;
}
