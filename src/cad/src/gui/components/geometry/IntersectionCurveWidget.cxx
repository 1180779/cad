//
// Created by Radosław Głasek on 01.08.2026
//

#include "IntersectionCurveWidget.hxx"

#include <cmath>

#include <QPainter>
#include <QSignalBlocker>

#include "../../../Scene.hpp"
#include "commands/CommandStack.hpp"
#include "commands/Commands.hpp"
#include "components/GeometryComponent.hpp"
#include "components/geometry/PatchComponent.hxx"
#include "factory/GeometryFactory.hpp"
#include "gui/Theme.hpp"
#include "gui/WidgetBuilders.hxx"

namespace {
    /// @brief Side of the square parameter-space previews, in pixels
    constexpr int gc_viewSide = 150;

    /// @brief Push the curve's masks onto its source surfaces, or clear them
    void applyTrimTo(Scene &scene, const IntersectionCurveComponent *curve, const bool on) {
        const auto apply = [&](const EntityId id, const trimming::TrimMask &mask, const bool keepInside) {
            const auto entity = scene.getEntity(id);
            if (!entity) {
                return;
            }
            if (const auto patch = entity.value()->getComponent<PatchComponent>()) {
                if (on) {
                    patch.value()->setTrim(mask, keepInside);
                }
                else {
                    patch.value()->clearTrim();
                }
            }
            else if (const auto torus = entity.value()->getComponent<TorusComponent>()) {
                if (on) {
                    torus.value()->setTrim(mask, keepInside);
                }
                else {
                    torus.value()->clearTrim();
                }
            }
        };
        const bool selfIntersection = curve->getPatch2() == curve->getPatch1();
        if (selfIntersection && !curve->getMask1().empty() && !curve->getMask2().empty()) {
            apply(
                curve->getPatch1(),
                trimming::combineKept(
                    curve->getMask1(),
                    curve->getKeepInside1(),
                    curve->getMask2(),
                    curve->getKeepInside2()
                ),
                true
            );
            return;
        }
        apply(curve->getPatch1(), curve->getMask1(), curve->getKeepInside1());
        if (!selfIntersection) {
            apply(curve->getPatch2(), curve->getMask2(), curve->getKeepInside2());
        }
    }
}

ParameterSpaceView::ParameterSpaceView(QWidget *parent)
: QWidget(parent) {
    setFixedSize(gc_viewSide, gc_viewSide);
    setToolTip("The intersection in this surface's (u, v); the lit region is kept");
}

void ParameterSpaceView::setCurve(
    const std::vector<cadm::Vec2> &params,
    const bool closed,
    const trimming::TrimMask &mask,
    const bool keepInside
) {
    m_closed = closed;
    // wrap each point into [0,1]^2 and start a new polyline wherever the curve
    // jumps across a periodic seam
    m_curves.clear();
    QPolygonF current;
    QPointF previous;
    for (const auto &p : params) {
        const auto wrapped = QPointF(
            p.x - std::floor(p.x),
            // v grows upwards, image rows grow downwards
            1.0 - (p.y - std::floor(p.y))
        );
        const bool jumped = !current.isEmpty()
            && (std::abs(wrapped.x() - previous.x()) > 0.5
                || std::abs(wrapped.y() - previous.y()) > 0.5);
        if (jumped) {
            m_curves.append(std::move(current));
            current = {};
        }
        current.append(wrapped);
        previous = wrapped;
    }
    if (!current.isEmpty()) {
        m_curves.append(std::move(current));
    }

    if (mask.empty()) {
        m_regions = {};
        update();
        return;
    }
    // one small image scaled at paint time, rather than a per-repaint rebuild
    m_regions = QImage(mask.size, mask.size, QImage::Format_ARGB32_Premultiplied);
    const auto &colors = theme::active();
    const auto kept = colors.accent;
    const auto dropped = colors.window;
    for (int y = 0; y < mask.size; ++y) {
        for (int x = 0; x < mask.size; ++x) {
            const bool isKept = trimming::kept(mask.at(x, y), keepInside);
            m_regions.setPixelColor(
                x,
                mask.size - 1 - y,
                // v grows upwards, image rows grow downwards
                isKept
                    ? kept
                    : dropped
            );
        }
    }
    update();
}

void ParameterSpaceView::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF area(0, 0, width(), height());
    if (m_regions.isNull()) {
        painter.fillRect(area, theme::active().window);
    }
    else {
        painter.drawImage(area, m_regions);
    }

    painter.setPen(QPen(theme::active().text, 1));
    painter.drawRect(area.adjusted(0, 0, -1, -1));

    if (m_curves.isEmpty()) {
        return;
    }
    painter.save();
    painter.scale(width(), height());
    painter.setPen(QPen(theme::active().curve, 2.0 / width()));
    if (m_closed && m_curves.size() == 1) {
        painter.drawPolygon(m_curves.first());
    }
    else {
        for (const auto &polyline : m_curves) {
            painter.drawPolyline(polyline);
        }
    }
    painter.restore();
}

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

    // one column per surface: parameter square & its flip button
    const auto views = new QHBoxLayout;
    const auto addColumn = [this, views](ParameterSpaceView *&view, const QString &title, const bool first) {
        const auto column = new QVBoxLayout;
        column->addWidget(new QLabel(title));
        view = new ParameterSpaceView;
        column->addWidget(view);
        const auto flip = new QPushButton("Flip side");
        column->addWidget(flip);
        const auto toggleAction = [this, first] {
            const auto toggle = [scene = m_scene, curve = m_curve, first, on = m_trim->isChecked()] {
                if (first) {
                    curve->setKeepInside1(!curve->getKeepInside1());
                }
                else {
                    curve->setKeepInside2(!curve->getKeepInside2());
                }
                if (scene && on) {
                    applyTrimTo(*scene, curve, true);
                }
            };
            pushEdit(toggle, toggle, nullptr);
            refreshViews();
        };
        connect(flip, &QPushButton::clicked, this, toggleAction);
        views->addLayout(column);
    };
    addColumn(m_view1, "Surface 1 (u, v)", true);
    addColumn(m_view2, "Surface 2 (u, v)", false);
    layout->addLayout(views);

    m_trim = widgets::addCheckbox(layout, "Trim surfaces", false);
    const auto trimToggleAction = [this](const bool on) {
        pushEdit(
            [scene = m_scene, curve = m_curve, on] {
                if (scene) {
                    applyTrimTo(*scene, curve, on);
                }
            },
            [scene = m_scene, curve = m_curve, on] {
                if (scene) {
                    applyTrimTo(*scene, curve, !on);
                }
            },
            m_trim
        );
    };
    connect(m_trim, &QCheckBox::toggled, this, trimToggleAction);

    m_stride = widgets::addSpinBox(layout, "Spline point every", 1, 500, 10);
    const auto toSpline = widgets::addButton(layout, "Convert to spline");
    connect(toSpline, &QPushButton::clicked, this, &IntersectionCurveWidget::convertToSpline);

    refreshViews();
}

void IntersectionCurveWidget::convertToSpline() const {
    if (!m_scene || !m_commandStack) {
        return;
    }
    const auto points = m_curve->getPoints3D();
    const int stride = m_stride->value();
    m_commandStack->push(
        std::make_unique<CreateEntitiesCommand>(
            *m_scene,
            [points, stride](Scene &s) {
                return GeometryFactory(s).createInterpolatedFromPoints(points, stride);
            }
        )
    );
}

void IntersectionCurveWidget::refreshViews() const {
    m_view1->setCurve(
        m_curve->getParams1(),
        m_curve->isClosed(),
        m_curve->getMask1(),
        m_curve->getKeepInside1()
    );
    m_view2->setCurve(
        m_curve->getParams2(),
        m_curve->isClosed(),
        m_curve->getMask2(),
        m_curve->getKeepInside2()
    );
}

void IntersectionCurveWidget::setCommandContext(Scene *scene, CommandStack *stack, const EntityId entityId) {
    ComponentWidget::setCommandContext(scene, stack, entityId);
    m_surface1Label->setText(surfaceLabel(m_curve->getPatch1()));
    m_surface2Label->setText(surfaceLabel(m_curve->getPatch2()));

    bool trimmed = false;
    if (m_scene) {
        if (const auto e = m_scene->getEntity(m_curve->getPatch1())) {
            if (const auto patch = e.value()->getComponent<PatchComponent>()) {
                trimmed = patch.value()->getTrim().enabled;
            }
            else if (const auto torus = e.value()->getComponent<TorusComponent>()) {
                trimmed = torus.value()->getTrim().enabled;
            }
        }
    }
    const QSignalBlocker block(m_trim);
    m_trim->setChecked(trimmed);
}

QString IntersectionCurveWidget::surfaceLabel(const EntityId id) const {
    if (m_scene) {
        if (const auto e = m_scene->getEntity(id)) {
            return QString::fromStdString(e.value()->getName());
        }
    }
    return QString("#%1").arg(id);
}
