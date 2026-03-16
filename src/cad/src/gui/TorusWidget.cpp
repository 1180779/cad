#include "TorusWidget.h"
#include <QLabel>

void TorusWidget::setUpMajorRadiusControls(QFormLayout *const layout)
{
    m_majorRadius = new QDoubleSpinBox();
    m_majorRadius->setMinimum(0.01);
    m_majorRadius->setValue(m_torus->m_majorRadius);
    m_majorRadius->setKeyboardTracking(true);
    connect(m_majorRadius, &QDoubleSpinBox::valueChanged, this, &TorusWidget::onMajorRadiusChanged);
    layout->addRow(new QLabel("Major Radius"), m_majorRadius);
}

void TorusWidget::setUpMinorRadiusControls(QFormLayout *const layout)
{
    m_minorRadius = new QDoubleSpinBox();
    m_minorRadius->setMinimum(0.01);
    m_minorRadius->setValue(m_torus->m_minorRadius);
    m_minorRadius->setKeyboardTracking(true);
    connect(m_minorRadius, &QDoubleSpinBox::valueChanged, this, &TorusWidget::onMinorRadiusChanged);
    layout->addRow(new QLabel("Minor Radius"), m_minorRadius);
}

void TorusWidget::setUpMajorSegmentsControls(QFormLayout *const layout)
{
    m_majorSegments = new QSpinBox();
    m_majorSegments->setMinimum(3);
    m_majorSegments->setMaximum(1000);
    m_majorSegments->setValue(static_cast<int>(m_torus->m_majorSegments));
    m_majorSegments->setKeyboardTracking(true);
    connect(m_majorSegments, &QSpinBox::valueChanged, this, &TorusWidget::onMajorSegmentsChanged);
    layout->addRow(new QLabel("Major Segments"), m_majorSegments);
}

void TorusWidget::setUpMinorSegmentsControls(QFormLayout *const layout)
{
    m_minorSegments = new QSpinBox();
    m_minorSegments->setMinimum(3);
    m_minorSegments->setMaximum(1000);
    m_minorSegments->setValue(static_cast<int>(m_torus->m_minorSegments));
    m_minorSegments->setKeyboardTracking(true);
    connect(m_minorSegments, &QSpinBox::valueChanged, this, &TorusWidget::onMinorSegmentsChanged);
    layout->addRow(new QLabel("Minor Segments"), m_minorSegments);
}

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
