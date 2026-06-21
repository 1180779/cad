#include "EntityPropertiesWidget.hpp"
#include "BezierC0Widget.hpp"
#include "BezierC2Widget.hpp"
#include "CadCameraWidget.hpp"
#include "PointPropertiesWidget.hpp"
#include "ProjectionCameraWidget.hpp"
#include "TorusWidget.hpp"
#include "TransformWidget.hpp"
#include "../components/BezierC0Component.hpp"
#include "../components/BezierC2Component.hpp"
#include "../components/BlenderCameraComponent.hpp"
#include "../components/CadCameraComponent.hpp"
#include "../components/GeometryComponent.hpp"
#include "../components/PointComponent.hpp"
#include "../components/TransformComponent.hpp"
#include <QVBoxLayout>

EntityPropertiesWidget::EntityPropertiesWidget(QWidget *parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setAlignment(Qt::AlignTop);
}

void EntityPropertiesWidget::setScene(Scene *scene) {
    m_scene = scene;
}

void EntityPropertiesWidget::setEntity(Entity *entity) {
    if (m_entity == entity) {
        return;
    }

    clearLayout();
    m_entity = entity;
    m_pointWidget = nullptr;
    m_bezierC0Widget = nullptr;
    m_bezierC2Widget = nullptr;

    if (!m_entity) {
        return;
    }

    if (const auto transform = m_entity->getComponent<TransformComponent>()) {
        const auto widget = new TransformWidget(transform.value());
        widget->setCommandContext(m_scene, m_commandStack, m_entity->getId());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto torus = m_entity->getComponent<TorusGeometry>()) {
        const auto widget = new TorusWidget(torus.value());
        widget->setCommandContext(m_scene, m_commandStack, m_entity->getId());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto camera = m_entity->getComponent<BlenderCameraComponent>()) {
        const auto widget = new CameraWidget(camera.value());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto camera = m_entity->getComponent<CadCameraComponent>()) {
        const auto widget = new CadCameraWidget(camera.value());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto pc = m_entity->getComponent<PointComponent>();
        pc && m_scene) {
        m_pointWidget = new PointPropertiesWidget(&m_scene->getPointRegistry());
        m_pointWidget->setCommandContext(m_scene, m_commandStack);
        m_pointWidget->setPoint(pc.value()->m_handle);
        m_layout->addWidget(m_pointWidget);
        connect(m_pointWidget, &PointPropertiesWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto bezier = m_entity->getComponent<BezierC0Component>()) {
        m_bezierC0Widget = new BezierC0Widget(bezier.value(), m_scene);
        m_bezierC0Widget->setCommandContext(m_scene, m_commandStack, m_entity->getId());
        m_layout->addWidget(m_bezierC0Widget);
        connect(m_bezierC0Widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
        connect(
            m_bezierC0Widget,
            &BezierC0Widget::pointSelectionChanged,
            this,
            &EntityPropertiesWidget::pointSelectionChanged
        );
    }

    if (const auto bezier = m_entity->getComponent<BezierC2Component>()) {
        m_bezierC2Widget = new BezierC2Widget(bezier.value(), m_scene);
        m_bezierC2Widget->setCommandContext(m_scene, m_commandStack, m_entity->getId());
        m_layout->addWidget(m_bezierC2Widget);
        connect(m_bezierC2Widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
        connect(
            m_bezierC2Widget,
            &BezierC2Widget::pointSelectionChanged,
            this,
            &EntityPropertiesWidget::pointSelectionChanged
        );
    }
}

void EntityPropertiesWidget::syncBezierSelection() const {
    if (m_bezierC0Widget) {
        m_bezierC0Widget->syncSelection();
    }
    if (m_bezierC2Widget) {
        m_bezierC2Widget->syncSelection();
    }
}

void EntityPropertiesWidget::refreshComponents() const {
    if (m_pointWidget) {
        m_pointWidget->refresh();
    }
    if (m_bezierC2Widget) {
        m_bezierC2Widget->refresh();
    }
}

void EntityPropertiesWidget::refreshComponentGeometry() const {
    if (m_pointWidget) {
        m_pointWidget->refresh();
    }
    if (m_bezierC2Widget) {
        m_bezierC2Widget->refreshGeometry();
    }
}

void EntityPropertiesWidget::clearLayout() const {
    QLayoutItem *item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        // deleteLater (not delete) as a safety net: the widget should not be deleted while
        // its own handler is on the call stack, but this prevents application crash in case
        // that happens for some reason (ex. due to future changes to the codebase)
        if (auto *w = item->widget()) {
            w->setParent(nullptr);
            w->deleteLater();
        }
        delete item;
    }
}
