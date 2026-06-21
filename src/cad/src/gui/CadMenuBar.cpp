//
// Created by Radosław Głasek on 21.06.2026
//

#include "CadMenuBar.hpp"

#include <QAction>
#include <QMenu>

#include "input/InputMap.hpp"

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
