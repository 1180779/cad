#ifndef COMPONENTWIDGET_H
#define COMPONENTWIDGET_H

#include <QWidget>

#include "../entities/entity.h"

class ComponentWidget : public QWidget
{
    Q_OBJECT

public:
    static constexpr int s_doubleSpinBoxFixedWidth = 80;

    explicit ComponentWidget(Component *component, QWidget *parent = nullptr);

    signals :
    
    void propertyChanged();

protected:
    Component *m_component;
};

#endif // COMPONENTWIDGET_H