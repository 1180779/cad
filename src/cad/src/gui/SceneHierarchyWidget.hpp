#ifndef SCENEHIERARCHYWIDGET_H
#define SCENEHIERARCHYWIDGET_H

#include <QList>
#include <QListWidgetItem>
#include "../entities/Entity.hpp"
#include "../Scene.hpp"
#include "../camera/CameraController.hpp"

class SceneHierarchyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneHierarchyWidget(QWidget *parent = nullptr);
    void setScene(Scene *scene);
    void setCameraController(CameraController *cameraController);
    void addEntityToList(const std::unique_ptr<Entity> &e) const;

    signals:

    void selectionChanged(QList<Entity*> entities);
    void deleteEntityRequested(Entity *e);
    void setAsCursorRequested(Entity *e);
    void setAsCameraRequested(EntityID id);
    void createTorusRequested();
    void createCursorRequested();

public slots:
    void refresh();

private slots:
    void onItemSelectionChanged();
    void onItemChanged(const QListWidgetItem *item) const;
    void onContextMenuRequested(const QPoint &pos);

private:
    void populateList();

    bool m_refreshing{false};

    Scene *m_scene = nullptr;
    CameraController *m_cameraController = nullptr;
    QListWidget *m_listWidget;
};

#endif // SCENEHIERARCHYWIDGET_H
