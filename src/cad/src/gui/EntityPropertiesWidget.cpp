#include "EntityPropertiesWidget.h"
#include "TransformWidget.h"
#include "TorusWidget.h"
#include "cameraWidget.hpp"
#include "../components/transform.h"
#include "../components/geometry.h"
#include "../components/camera.hpp"
#include <QVBoxLayout>

EntityPropertiesWidget::EntityPropertiesWidget(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setAlignment(Qt::AlignTop);
}

void EntityPropertiesWidget::setEntity(entity *entity)
{
    if (m_entity == entity)
        return;

    clearLayout();
    m_entity = entity;

    if (!m_entity)
        return;

    if (const auto transform = m_entity->getComponent<TransformComponent>())
    {
        const auto widget = new TransformWidget(transform.value());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto torus = m_entity->getComponent<TorusGeometry>())
    {
        const auto widget = new TorusWidget(torus.value());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto camera = m_entity->getComponent<CameraComponent>())
    {
        const auto widget = new CameraWidget(camera.value());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }
}

void EntityPropertiesWidget::clearLayout() const
{
    QLayoutItem *item;
    while ((item = m_layout->takeAt(0)) != nullptr)
    {
        delete item->widget();
        delete item;
    }
}