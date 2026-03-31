#ifndef ENTITYPROPERTIESWIDGET_H
#define ENTITYPROPERTIESWIDGET_H

#include <QWidget>
#include "../entities/Entity.hpp"
#include "../Scene.hpp"

class QVBoxLayout;

class EntityPropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EntityPropertiesWidget(QWidget *parent = nullptr);
    void setScene(Scene *scene);
    void setEntity(Entity *entity);

    signals  :



    void propertyChanged();

private:
    void clearLayout() const;

    Scene *m_scene = nullptr;
    Entity *m_entity = nullptr;
    QVBoxLayout *m_layout;
};

#endif // ENTITYPROPERTIESWIDGET_H