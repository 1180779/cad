//
// Created by Radosław Głasek on 01.08.2026
//

#include "IntersectionCurveWidget.hxx"

#include "../../../Scene.hpp"
#include "gui/WidgetBuilders.hxx"

IntersectionCurveWidget::IntersectionCurveWidget(IntersectionCurveComponent *curve, QWidget *parent)
: ComponentWidget(curve, parent),
  m_curve(curve) {
    const auto layout = new QVBoxLayout(this);
    widgets::addTitle(layout, "Intersection Curve");

    const auto form = new QFormLayout;
    form->addRow("Points", new QLabel(QString::number(m_curve->getPoints3D().size())));
    form->addRow(
        "Closed",
        new QLabel(
            m_curve->isClosed()
                ? "Yes"
                : "No"
        )
    );
    m_surface1Label = new QLabel(surfaceLabel(m_curve->getPatch1()));
    m_surface2Label = new QLabel(surfaceLabel(m_curve->getPatch2()));
    form->addRow("Surface 1", m_surface1Label);
    form->addRow("Surface 2", m_surface2Label);
    layout->addLayout(form);
}

void IntersectionCurveWidget::setCommandContext(Scene *scene, CommandStack *stack, const EntityId entityId) {
    ComponentWidget::setCommandContext(scene, stack, entityId);
    m_surface1Label->setText(surfaceLabel(m_curve->getPatch1()));
    m_surface2Label->setText(surfaceLabel(m_curve->getPatch2()));
}

QString IntersectionCurveWidget::surfaceLabel(const EntityId id) const {
    if (m_scene) {
        if (const auto e = m_scene->getEntity(id)) {
            return QString::fromStdString(e.value()->getName());
        }
    }
    return QString("#%1").arg(id);
}
