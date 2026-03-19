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
}

void TorusWidget::onMajorRadiusChanged(const double value) const
{
    m_torus->m_majorRadius = static_cast<cadm::cadf>(value);
    m_torus->m_needsUpdate = true;
    m_minorRadius->setMaximum(m_torus->m_majorRadius - s_minorRadiusMin);
    emit const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMinorRadiusChanged(const double value) const
{
    m_torus->m_minorRadius = static_cast<cadm::cadf>(value);
    m_torus->m_needsUpdate = true;
    emit const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMajorSegmentsChanged(const int value) const
{
    m_torus->m_majorSegments = static_cast<uint32_t>(value);
    m_torus->m_needsUpdate = true;
    emit const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMinorSegmentsChanged(const int value) const
{
    m_torus->m_minorSegments = static_cast<uint32_t>(value);
    m_torus->m_needsUpdate = true;
    emit const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::setUpMajorRadiusControls(QFormLayout *const layout)
{
    m_majorRadius = new QDoubleSpinBox();
    m_majorRadius->setMinimum(s_majorRadiusMin);
    m_majorRadius->setMaximum(s_majorRadiusMax);
    m_majorRadius->setSingleStep(s_majorRadiusStep);
    m_majorRadius->setValue(m_torus->m_majorRadius);
    m_majorRadius->setKeyboardTracking(true);
    m_majorRadius->setFixedWidth(s_doubleSpinBoxFixedWidth);
    connect(m_majorRadius, &QDoubleSpinBox::valueChanged, this, &TorusWidget::onMajorRadiusChanged);
    layout->addRow(new QLabel("Major Radius"), m_majorRadius);
}

void TorusWidget::setUpMinorRadiusControls(QFormLayout *const layout)
{
    m_minorRadius = new QDoubleSpinBox();
    m_minorRadius->setMinimum(s_minorRadiusMin);
    m_minorRadius->setMaximum(m_torus->m_majorRadius - s_minorRadiusMin);
    m_minorRadius->setSingleStep(s_minorRadiusStep);
    m_minorRadius->setValue(m_torus->m_minorRadius);
    m_minorRadius->setKeyboardTracking(true);
    m_minorRadius->setFixedWidth(s_doubleSpinBoxFixedWidth);
    connect(m_minorRadius, &QDoubleSpinBox::valueChanged, this, &TorusWidget::onMinorRadiusChanged);
    layout->addRow(new QLabel("Minor Radius"), m_minorRadius);
}

void TorusWidget::setUpMajorSegmentsControls(QFormLayout *const layout)
{
    m_majorSegments = new QSpinBox();
    m_majorSegments->setMinimum(s_majorSegmentsMin);
    m_majorSegments->setMaximum(s_majorSegmentsMax);
    m_majorSegments->setValue(static_cast<int>(m_torus->m_majorSegments));
    m_majorSegments->setKeyboardTracking(true);
    m_majorSegments->setFixedWidth(s_doubleSpinBoxFixedWidth);
    connect(m_majorSegments, &QSpinBox::valueChanged, this, &TorusWidget::onMajorSegmentsChanged);
    layout->addRow(new QLabel("Major Segments"), m_majorSegments);
}

void TorusWidget::setUpMinorSegmentsControls(QFormLayout *const layout)
{
    m_minorSegments = new QSpinBox();
    m_minorSegments->setMinimum(s_minorSegmentsMin);
    m_minorSegments->setMaximum(s_minorSegmentsMax);
    m_minorSegments->setValue(static_cast<int>(m_torus->m_minorSegments));
    m_minorSegments->setKeyboardTracking(true);
    m_minorSegments->setFixedWidth(s_doubleSpinBoxFixedWidth);
    connect(m_minorSegments, &QSpinBox::valueChanged, this, &TorusWidget::onMinorSegmentsChanged);
    layout->addRow(new QLabel("Minor Segments"), m_minorSegments);
}
