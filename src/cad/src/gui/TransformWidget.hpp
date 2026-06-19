#ifndef TRANSFORMWIDGET_H
#define TRANSFORMWIDGET_H

#include <QFormLayout>

#include "ComponentWidget.hpp"
#include <common/ModifierSpinBox.hpp>
#include "../components/TransformComponent.hpp"

class TransformWidget : public ComponentWidget {
    Q_OBJECT

public:
    explicit TransformWidget(TransformComponent *transform, QWidget *parent = nullptr);

private
    slots  :

    

    void onTranslationXChanged(double value) const;

    void onTranslationYChanged(double value) const;

    void onTranslationZChanged(double value) const;

    void onScaleXChanged(double value) const;

    void onScaleYChanged(double value) const;

    void onScaleZChanged(double value) const;

    void onRotationXChanged(double value) const;

    void onRotationYChanged(double value) const;

    void onRotationZChanged(double value) const;

private:
    /// @brief Build one axis spin box with the shared styling and the given
    /// per-group range/step/wrapping and initial value
    static ModifierDoubleSpinBox* makeAxisSpin(
        double step,
        double min,
        double max,
        bool wrapping,
        double value
    );

    static constexpr double s_angleMin = -180.0;
    static constexpr double s_angleMax = 180.0;
    static constexpr double s_angleStep = 5.0;

    static constexpr double s_translationMin = std::numeric_limits<double>::lowest();
    static constexpr double s_translationMax = std::numeric_limits<double>::max();
    static constexpr double s_translationStep = 0.1;

    static constexpr double s_scaleMin = 0.1;
    static constexpr double s_scaleMax = 1000.0;
    static constexpr double s_scaleStep = 0.1;

    TransformComponent *m_transform;

    ModifierDoubleSpinBox *m_translationX;
    ModifierDoubleSpinBox *m_translationY;
    ModifierDoubleSpinBox *m_translationZ;

    ModifierDoubleSpinBox *m_scaleX;
    ModifierDoubleSpinBox *m_scaleY;
    ModifierDoubleSpinBox *m_scaleZ;

    ModifierDoubleSpinBox *m_rotationX;
    ModifierDoubleSpinBox *m_rotationY;
    ModifierDoubleSpinBox *m_rotationZ;
};

#endif // TRANSFORMWIDGET_H
