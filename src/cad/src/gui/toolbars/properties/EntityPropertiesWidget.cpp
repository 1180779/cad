#include "EntityPropertiesWidget.hpp"
#include "../../components/geometry/BezierC0Widget.hpp"
#include "../../components/geometry/BezierC2Widget.hpp"
#include "../../components/geometry/InterpC2Widget.hxx"
#include "../../components/camera/CadCameraWidget.hpp"
#include "../../components/geometry/PointPropertiesWidget.hpp"
#include "../../components/camera/ProjectionCameraWidget.hpp"
#include "../../components/geometry/TorusWidget.hpp"
#include "../../components/TransformWidget.hpp"
#include "../../components/geometry/GregoryWidget.hxx"
#include "../../components/geometry/PatchWidget.hxx"
#include "../../../components/geometry/BezierC0Component.hpp"
#include "../../../components/geometry/BezierC2Component.hpp"
#include "../../../components/geometry/InterpC2Component.hxx"
#include "../../../components/geometry/PatchC0Component.hxx"
#include "../../../components/camera/BlenderCameraComponent.hpp"
#include "../../../components/camera/CadCameraComponent.hpp"
#include "../../../components/GeometryComponent.hpp"
#include "../../../components/PointComponent.hpp"
#include "../../../components/TransformComponent.hpp"
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
    m_entityId = entity
                     ? entity->getId()
                     : 0;
    m_pointWidget = nullptr;
    m_bezierC0Widget = nullptr;
    m_bezierC2Widget = nullptr;
    m_interpC2Widget = nullptr;

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

    if (const auto curve = m_entity->getComponent<InterpC2Component>()) {
        m_interpC2Widget = new InterpC2Widget(curve.value(), m_scene);
        m_interpC2Widget->setCommandContext(m_scene, m_commandStack, m_entity->getId());
        m_layout->addWidget(m_interpC2Widget);
        connect(m_interpC2Widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
        connect(
            m_interpC2Widget,
            &InterpC2Widget::pointSelectionChanged,
            this,
            &EntityPropertiesWidget::pointSelectionChanged
        );
    }

    if (const auto patch = m_entity->getComponent<PatchComponent>()) {
        const QString title = m_entity->hasComponent<PatchC0Component>()
                                  ? "Bézier Patch C0"
                                  : "Bézier Patch C2";
        const auto widget = new PatchWidget(patch.value(), title);
        widget->setCommandContext(m_scene, m_commandStack, m_entity->getId());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }

    if (const auto gregory = m_entity->getComponent<GregoryComponent>()) {
        const auto widget = new GregoryWidget(gregory.value());
        widget->setCommandContext(m_scene, m_commandStack, m_entity->getId());
        m_layout->addWidget(widget);
        connect(widget, &ComponentWidget::propertyChanged, this, &EntityPropertiesWidget::propertyChanged);
    }
}

bool EntityPropertiesWidget::validateEntity() {
    if (!m_entity) {
        return false;
    }
    if (m_scene) {
        if (const auto e = m_scene->getEntity(m_entityId);
            !e || e.value() != m_entity) {
            setEntity(nullptr);
            return false;
        }
    }
    return true;
}

void EntityPropertiesWidget::syncBezierSelection() {
    if (!validateEntity()) {
        return;
    }
    if (m_bezierC0Widget) {
        m_bezierC0Widget->syncSelection();
    }
    if (m_bezierC2Widget) {
        m_bezierC2Widget->syncSelection();
    }
    if (m_interpC2Widget) {
        m_interpC2Widget->syncSelection();
    }
}

void EntityPropertiesWidget::refreshComponents() {
    if (!validateEntity()) {
        return;
    }
    if (m_pointWidget) {
        m_pointWidget->refresh();
    }
    if (m_bezierC2Widget) {
        m_bezierC2Widget->refresh();
    }
    if (m_interpC2Widget) {
        m_interpC2Widget->refresh();
    }
}

void EntityPropertiesWidget::refreshComponentGeometry() {
    if (!validateEntity()) {
        return;
    }
    if (m_pointWidget) {
        m_pointWidget->refresh();
    }
    if (m_bezierC2Widget) {
        m_bezierC2Widget->refreshGeometry();
    }
    if (m_interpC2Widget) {
        m_interpC2Widget->refreshGeometry();
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
