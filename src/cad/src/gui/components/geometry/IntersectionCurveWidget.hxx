//
// Created by Radosław Głasek on 01.08.2026
//

#ifndef CAD_INTERSECTIONCURVEWIDGET_HXX
#define CAD_INTERSECTIONCURVEWIDGET_HXX

#include <QLabel>

#include "../ComponentWidget.hpp"
#include "../../../components/geometry/IntersectionCurveComponent.hxx"

/// @brief Read-only summary of an IntersectionCurveComponent: point count,
/// open/closed, and the two source surfaces
class IntersectionCurveWidget final : public ComponentWidget {
    Q_OBJECT

public:
    explicit IntersectionCurveWidget(IntersectionCurveComponent *curve, QWidget *parent = nullptr);

    void setCommandContext(Scene *scene, CommandStack *stack, EntityId entityId) override;

private:
    [[nodiscard]] QString surfaceLabel(EntityId id) const;

    IntersectionCurveComponent *m_curve;
    QLabel *m_surface1Label{};
    QLabel *m_surface2Label{};
};

#endif //CAD_INTERSECTIONCURVEWIDGET_HXX
