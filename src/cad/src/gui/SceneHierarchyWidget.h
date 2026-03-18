#ifndef SCENEHIERARCHYWIDGET_H
#define SCENEHIERARCHYWIDGET_H

#include <QListWidgetItem>
#include "../entities/entity.h"
#include "../scene.h"

class SceneHierarchyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneHierarchyWidget(QWidget *parent = nullptr);
    void setScene(Scene *scene);

    signals :
    
    void entitySelected(entity *entity);

private
    slots :
    
    void onCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void populateList();

    Scene *m_scene = nullptr;
    QListWidget *m_listWidget;
};

#endif // SCENEHIERARCHYWIDGET_H