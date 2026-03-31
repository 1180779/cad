#include "EntityPropertiesWidget.hpp"
#include "TransformWidget.hpp"
#include "TorusWidget.hpp"
#include "ProjectionCameraWidget.hpp"
#include "PointPropertiesWidget.hpp"
#include "../components/TransformComponent.hpp"
#include "../components/GeometryComponent.hpp"
#include "../components/ProjectionCameraComponent.hpp"
#include "../components/PointComponent.hpp"
#include <QVBoxLayout>

#include "CadCameraWidget.hpp"
#include "../components/CadCameraComponent.hpp"

EntityPropertiesWidget::EntityPropertiesWidget(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setAlignment(Qt::AlignTop);
}

void EntityPropertiesWidget::setScene(Scene *scene)
{
    m_scene = scene;
}

void EntityPropertiesWidget::setEntity(Entity *entity)
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

    if (const auto camera = m_entity->getComponent<ProjectionCameraComponent>())
    {
        const auto widget = new CameraWidget(camera.value());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto camera = m_entity->getComponent<CadCameraComponent>())
    {
        const auto widget = new CadCameraWidget(camera.value());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto pc = m_entity->getComponent<PointComponent>(); pc && m_scene)
    {
        const auto widget = new PointPropertiesWidget();
        widget->setPoint(&m_scene->getPointRegistry(), pc.value()->m_handle);
        m_layout->addWidget(widget);
        connect(widget, &PointPropertiesWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
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