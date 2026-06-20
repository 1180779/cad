#include "CadMenuBar.hpp"

#include <QAction>
#include <QMenu>

CadMenuBar::CadMenuBar(QWidget *parent) : QMenuBar(parent) {
    m_editMenu = addMenu("Edit");
    m_undoAction = m_editMenu->addAction("Undo", this, &CadMenuBar::undoRequested);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_redoAction = m_editMenu->addAction("Redo", this, &CadMenuBar::redoRequested);
    m_redoAction->setShortcut(QKeySequence::Redo);

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

void CadMenuBar::setUndoEnabled(const bool enabled) {
    m_undoAction->setEnabled(enabled);
}

void CadMenuBar::setRedoEnabled(const bool enabled) {
    m_redoAction->setEnabled(enabled);
}

QAction* CadMenuBar::addToolPanelAction(const QString &name) {
    auto *action = m_toolsMenu->addAction(name);
    action->setCheckable(true);
    action->setChecked(true);
    return action;
}
