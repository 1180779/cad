#include "EntityPropertiesWidget.hpp"
#include "BezierC0Widget.hpp"
#include "CadCameraWidget.hpp"
#include "PointPropertiesWidget.hpp"
#include "ProjectionCameraWidget.hpp"
#include "TorusWidget.hpp"
#include "TransformWidget.hpp"
#include "../components/BezierC0Component.hpp"
#include "../components/BlenderCameraComponent.hpp"
#include "../components/CadCameraComponent.hpp"
#include "../components/GeometryComponent.hpp"
#include "../components/PointComponent.hpp"
#include "../components/TransformComponent.hpp"
#include <QVBoxLayout>

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
    m_bezierWidget = nullptr;

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

    if (const auto camera = m_entity->getComponent<BlenderCameraComponent>())
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

    if (const auto bezier = m_entity->getComponent<BezierC0Component>())
    {
        m_bezierWidget = new BezierC0Widget(bezier.value(), m_scene);
        m_layout->addWidget(m_bezierWidget);
        connect(m_bezierWidget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
        connect(
            m_bezierWidget,
            &BezierC0Widget::pointSelectionChanged,
            this,
            &EntityPropertiesWidget::pointSelectionChanged);
    }
}

void EntityPropertiesWidget::syncBezierSelection() const
{
    if (m_bezierWidget)
        m_bezierWidget->syncSelection();
}

void EntityPropertiesWidget::refreshComponents() const
{
    if (m_bezierWidget)
        m_bezierWidget->refreshList();
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
