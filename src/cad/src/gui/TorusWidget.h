#ifndef TORUSWIDGET_H
#define TORUSWIDGET_H

#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>

#include "ComponentWidget.h"
#include "../components/geometry.h"

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

    TorusGeometry *m_torus;

    QDoubleSpinBox *m_majorRadius;
    QDoubleSpinBox *m_minorRadius;
    QSpinBox *m_majorSegments;
    QSpinBox *m_minorSegments;
};

#endif // TORUSWIDGET_H
