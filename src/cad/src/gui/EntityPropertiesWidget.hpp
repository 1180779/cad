#ifndef ENTITYPROPERTIESWIDGET_H
#define ENTITYPROPERTIESWIDGET_H

#include <QVBoxLayout>
#include <QWidget>
#include "../components/Entity.hpp"
#include "../Scene.hpp"

class EntityPropertiesWidget : public QWidget {
    Q_OBJECT

public:
    explicit EntityPropertiesWidget(QWidget *parent = nullptr);

    void setScene(Scene *scene);

    void setEntity(Entity *entity);

signals:
    void propertyChanged();

    void pointSelectionChanged(QList<Entity*> selected);

public slots:
    void syncBezierSelection() const;

    void refreshComponents() const;

private:
    void clearLayout() const;

    Scene *m_scene = nullptr;
    Entity *m_entity = nullptr;
    QVBoxLayout *m_layout;
    class PointPropertiesWidget *m_pointWidget = nullptr;
    class BezierC0Widget *m_bezierWidget = nullptr;
    class BezierC2Widget *m_bezierC2Widget = nullptr;
};

#endif // ENTITYPROPERTIESWIDGET_H
