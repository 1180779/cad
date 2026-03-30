#ifndef SCENEHIERARCHYWIDGET_H
#define SCENEHIERARCHYWIDGET_H

#include <QList>
#include <QListWidgetItem>
#include "../entities/entity.hpp"
#include "../scene.hpp"
#include "../camera/CameraController.hpp"

class SceneHierarchyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneHierarchyWidget(QWidget *parent = nullptr);
    void setScene(Scene *scene);
    void setCameraController(CameraController *cameraController);
    void addEntityToList(const std::unique_ptr<entity> &e) const;

signals:
    void selectionChanged(QList<entity*> entities);
    void deleteEntityRequested(entity *e);
    void setAsCursorRequested(entity *e);
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
