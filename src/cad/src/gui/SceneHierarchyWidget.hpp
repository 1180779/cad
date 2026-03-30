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
    void addEntityToList(const std::unique_ptr<entity> &e) const;

signals:
    void selectionChanged(QList<entity*> entities);

public slots:
    void refresh();

private slots:
    void onItemSelectionChanged();
    void onItemChanged(const QListWidgetItem *item) const;

private:
    void populateList();

    bool m_refreshing{false};

    Scene *m_scene = nullptr;
    QListWidget *m_listWidget;
};

#endif // SCENEHIERARCHYWIDGET_H
