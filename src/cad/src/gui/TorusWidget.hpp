#ifndef TORUSWIDGET_H
#define TORUSWIDGET_H

#include <QFormLayout>

#include "ComponentWidget.hpp"
#include <common/ModifierSpinBox.hpp>
#include "../components/GeometryComponent.hpp"

class TorusWidget : public ComponentWidget
{
    Q_OBJECT

public:
    explicit TorusWidget(TorusGeometry *torus, QWidget *parent = nullptr);

private slots:
    void onMajorRadiusChanged(double value) const;
    void onMinorRadiusChanged(double value) const;
    void onMajorSegmentsChanged(int value) const;
    void onMinorSegmentsChanged(int value) const;

private:
    void setUpMajorRadiusControls(QFormLayout *layout);
    void setUpMinorRadiusControls(QFormLayout *layout);
    void setUpMajorSegmentsControls(QFormLayout *layout);
    void setUpMinorSegmentsControls(QFormLayout *layout);

    static constexpr double s_minorRadiusMin = 0.1;
    static constexpr double s_minorRadiusStep = 0.1;

    static constexpr double s_majorRadiusMin = 0.1 + s_minorRadiusMin;
    static constexpr double s_majorRadiusMax = 1000.0;
    static constexpr double s_majorRadiusStep = 0.1;

    static constexpr int s_majorSegmentsMin = 3;
    static constexpr int s_majorSegmentsMax = 1000;

    static constexpr int s_minorSegmentsMin = 3;
    static constexpr int s_minorSegmentsMax = 1000;

    TorusGeometry *m_torus;

    ModifierDoubleSpinBox *m_majorRadius{};
    ModifierDoubleSpinBox *m_minorRadius{};
    ModifierSpinBox *m_majorSegments{};
    ModifierSpinBox *m_minorSegments{};
};

#endif // TORUSWIDGET_H
