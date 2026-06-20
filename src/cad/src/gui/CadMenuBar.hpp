#ifndef CAD_CADMENUBAR_HPP
#define CAD_CADMENUBAR_HPP

#include <QMenuBar>

class QAction;

class CadMenuBar final : public QMenuBar {
    Q_OBJECT

public:
    explicit CadMenuBar(QWidget *parent = nullptr);

    void setUndoEnabled(bool enabled);

    void setRedoEnabled(bool enabled);

    /// @brief Register a toggleable panel entry under the Tools menu; returns the action
    QAction* addToolPanelAction(const QString &name);

signals:
    void undoRequested();

    void redoRequested();

    /// @brief Emitted just before the Edit menu opens so callers can refresh enabled state
    void editMenuAboutToShow();

private:
    QMenu *m_editMenu;
    QMenu *m_toolsMenu;
    QAction *m_undoAction;
    QAction *m_redoAction;
};

#endif
