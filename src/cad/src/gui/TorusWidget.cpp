#include "TorusWidget.h"
#include <QLabel>

TorusWidget::TorusWidget(TorusGeometry *torus, QWidget *parent)
    : ComponentWidget(torus, parent), m_torus(torus)
{
    const auto layout = new QFormLayout(this);

    setUpMajorRadiusControls(layout);
    setUpMinorRadiusControls(layout);
    setUpMajorSegmentsControls(layout);
    setUpMinorSegmentsControls(layout);

    connect(m_majorRadius, &QDoubleSpinBox::valueChanged, m_torus, &TorusGeometry::setMajorRadius);
    connect(m_minorRadius, &QDoubleSpinBox::valueChanged, m_torus, &TorusGeometry::setMinorRadius);
    connect(m_majorSegments, &QSpinBox::valueChanged, m_torus, &TorusGeometry::setMajorSegments);
    connect(m_minorSegments, &QSpinBox::valueChanged, m_torus, &TorusGeometry::setMinorSegments);

    connect(m_torus, &TorusGeometry::majorRadiusChanged, this, &TorusWidget::onMajorRadiusChanged);
    connect(m_torus, &TorusGeometry::minorRadiusChanged, this, &TorusWidget::onMinorRadiusChanged);
    connect(m_torus, &TorusGeometry::majorSegmentsChanged, this, &TorusWidget::onMajorSegmentsChanged);
    connect(m_torus, &TorusGeometry::minorSegmentsChanged, this, &TorusWidget::onMinorSegmentsChanged);
}

void TorusWidget::onMajorRadiusChanged(const double value) const
{
    m_majorRadius->setValue(value);
    m_minorRadius->setMaximum(value - s_minorRadiusMin);
    emit const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMinorRadiusChanged(const double value) const
{
    m_minorRadius->setValue(value);
    emit const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMajorSegmentsChanged(const int value) const
{
    m_majorSegments->setValue(value);
    emit const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMinorSegmentsChanged(const int value) const
{
    m_minorSegments->setValue(value);
    emit const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::setUpMajorRadiusControls(QFormLayout *const layout)
{
    m_majorRadius = new QDoubleSpinBox();
    m_majorRadius->setMinimum(s_majorRadiusMin);
    m_majorRadius->setMaximum(s_majorRadiusMax);
    m_majorRadius->setSingleStep(s_majorRadiusStep);
    m_majorRadius->setValue(m_torus->getMajorRadius());
    m_majorRadius->setKeyboardTracking(true);
    m_majorRadius->setFixedWidth(s_doubleSpinBoxFixedWidth);
    layout->addRow(new QLabel("Major Radius"), m_majorRadius);
}

void TorusWidget::setUpMinorRadiusControls(QFormLayout *const layout)
{
    m_minorRadius = new QDoubleSpinBox();
    m_minorRadius->setMinimum(s_minorRadiusMin);
    m_minorRadius->setMaximum(m_torus->getMajorRadius() - s_minorRadiusMin);
    m_minorRadius->setSingleStep(s_minorRadiusStep);
    m_minorRadius->setValue(m_torus->getMinorRadius());
    m_minorRadius->setKeyboardTracking(true);
    m_minorRadius->setFixedWidth(s_doubleSpinBoxFixedWidth);
    layout->addRow(new QLabel("Minor Radius"), m_minorRadius);
}

void TorusWidget::setUpMajorSegmentsControls(QFormLayout *const layout)
{
    m_majorSegments = new QSpinBox();
    m_majorSegments->setMinimum(s_majorSegmentsMin);
    m_majorSegments->setMaximum(s_majorSegmentsMax);
    m_majorSegments->setValue(static_cast<int>(m_torus->getMajorSegments()));
    m_majorSegments->setKeyboardTracking(true);
    m_majorSegments->setFixedWidth(s_doubleSpinBoxFixedWidth);
    layout->addRow(new QLabel("Major Segments"), m_majorSegments);
}

void TorusWidget::setUpMinorSegmentsControls(QFormLayout *const layout)
{
    m_minorSegments = new QSpinBox();
    m_minorSegments->setMinimum(s_minorSegmentsMin);
    m_minorSegments->setMaximum(s_minorSegmentsMax);
    m_minorSegments->setValue(static_cast<int>(m_torus->getMinorSegments()));
    m_minorSegments->setKeyboardTracking(true);
    m_minorSegments->setFixedWidth(s_doubleSpinBoxFixedWidth);
    layout->addRow(new QLabel("Minor Segments"), m_minorSegments);
}
