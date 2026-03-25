#ifndef SCENEHIERARCHYWIDGET_H
#define SCENEHIERARCHYWIDGET_H

#include <QList>
#include <QListWidgetItem>
#include "../entities/entity.hpp"
#include "../scene.hpp"

class SceneHierarchyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneHierarchyWidget(QWidget *parent = nullptr);
    void setScene(Scene *scene);

signals :
    void selectionChanged(QList<entity*> entities);

private
slots :
    void onItemSelectionChanged();

private:
    void populateList() const;

    Scene *m_scene = nullptr;
    QListWidget *m_listWidget;
};

#endif // SCENEHIERARCHYWIDGET_H
