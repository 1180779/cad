#ifndef ENTITYPROPERTIESWIDGET_H
#define ENTITYPROPERTIESWIDGET_H

#include <QWidget>
#include "../components/Entity.hpp"
#include "../Scene.hpp"

class QVBoxLayout;

class EntityPropertiesWidget : public QWidget
{
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
    class BezierC0Widget *m_bezierWidget = nullptr;
};

#endif // ENTITYPROPERTIESWIDGET_H
