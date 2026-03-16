#include "ComponentWidget.h"

ComponentWidget::ComponentWidget(Component *component, QWidget *parent)
    : QWidget(parent), m_component(component)
{
}
