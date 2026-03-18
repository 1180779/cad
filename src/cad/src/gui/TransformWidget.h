#ifndef TRANSFORMWIDGET_H
#define TRANSFORMWIDGET_H

#include <QFormLayout>
#include <QDoubleSpinBox>

#include "ComponentWidget.h"
#include "../components/transform.h"

class TransformWidget : public ComponentWidget
{
    Q_OBJECT

public:
    explicit TransformWidget(TransformComponent *transform, QWidget *parent = nullptr);

private slots:
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
    void setUpTranslationControls(QFormLayout *layout);
    void setUpScaleControls(QFormLayout *layout);
    void setUpRotationControls(QFormLayout *layout);

    static constexpr double s_angleMin = 0.0;
    static constexpr double s_angleMax = 360.0;
    static constexpr double s_angleStep = 5.0;

    static constexpr double s_translationMin = std::numeric_limits<double>::lowest();
    static constexpr double s_translationMax = std::numeric_limits<double>::max();
    static constexpr double s_translationStep = 0.1;

    static constexpr double s_scaleMin = 0.1;
    static constexpr double s_scaleMax = 1000.0;
    static constexpr double s_scaleStep = 0.1;

    TransformComponent *m_transform;

    QDoubleSpinBox *m_translationX;
    QDoubleSpinBox *m_translationY;
    QDoubleSpinBox *m_translationZ;

    QDoubleSpinBox *m_scaleX;
    QDoubleSpinBox *m_scaleY;
    QDoubleSpinBox *m_scaleZ;

    QDoubleSpinBox *m_rotationX;
    QDoubleSpinBox *m_rotationY;
    QDoubleSpinBox *m_rotationZ;
};

#endif // TRANSFORMWIDGET_H
