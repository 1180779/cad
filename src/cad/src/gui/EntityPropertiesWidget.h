#ifndef ENTITYPROPERTIESWIDGET_H
#define ENTITYPROPERTIESWIDGET_H

#include <QWidget>
#include "../entities/entity.h"

class QVBoxLayout;

class EntityPropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EntityPropertiesWidget(QWidget *parent = nullptr);
    void setEntity(entity *entity);

    signals :
    
    void propertyChanged();

private:
    void clearLayout() const;

    entity *m_entity = nullptr;
    QVBoxLayout *m_layout;
};

#endif // ENTITYPROPERTIESWIDGET_H