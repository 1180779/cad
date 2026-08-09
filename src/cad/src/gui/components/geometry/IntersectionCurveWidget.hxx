//
// Created by Radosław Głasek on 01.08.2026
//

#ifndef CAD_INTERSECTIONCURVEWIDGET_HXX
#define CAD_INTERSECTIONCURVEWIDGET_HXX

#include <QCheckBox>
#include <QImage>
#include <QLabel>
#include <QPolygonF>
#include <QSpinBox>

#include "../ComponentWidget.hpp"
#include "../../../components/geometry/IntersectionCurveComponent.hxx"

/// @brief The curve drawn in one surface's (u, v) domain, over the two regions
/// it splits it into
class ParameterSpaceView final : public QWidget {
    Q_OBJECT

public:
    explicit ParameterSpaceView(QWidget *parent = nullptr);

    /// @param params curve points in this surface's (u, v)
    /// @param closed whether the curve joins back to its start
    /// @param mask the parameter domain split along the curve
    /// @param keepInside which region is drawn as kept
    void setCurve(
        const std::vector<cadm::Vec2> &params,
        bool closed,
        const trimming::TrimMask &mask,
        bool keepInside
    );

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    /// @brief The mask rendered once per change rather than per repaint
    QImage m_regions;
    /// @brief The curve wrapped into the unit square, split where it crosses a
    /// periodic seam
    QList<QPolygonF> m_curves;
    bool m_closed = false;
};

/// @brief Read-only summary of an IntersectionCurveComponent: point count,
/// open/closed, the two source surfaces, and the curve in both parameter spaces
class IntersectionCurveWidget final : public ComponentWidget {
    Q_OBJECT

public:
    explicit IntersectionCurveWidget(IntersectionCurveComponent *curve, QWidget *parent = nullptr);

    void setCommandContext(Scene *scene, CommandStack *stack, EntityId entityId) override;

private:
    [[nodiscard]] QString surfaceLabel(EntityId id) const;

    /// @brief Push the component's current masks and kept sides into the views
    void refreshViews() const;

    /// @brief Create an interpolating C2 spline through the traced points
    void convertToSpline() const;

    IntersectionCurveComponent *m_curve;
    QLabel *m_surface1Label{};
    QLabel *m_surface2Label{};
    ParameterSpaceView *m_view1{};
    ParameterSpaceView *m_view2{};
    QCheckBox *m_trim{};
    QSpinBox *m_stride{};
};

#endif //CAD_INTERSECTIONCURVEWIDGET_HXX
