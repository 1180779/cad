//
// Created on 3/6/26.
//

#include "../include/common/DoubleSlider.h"

DoubleSlider::DoubleSlider(QWidget *parent)
    : QSlider(parent)
{
    connect(this, &QSlider::valueChanged, this, &DoubleSlider::notifyValueChanged);
}

void DoubleSlider::setMappingRange(const double start, const double end)
{
    if (start > end)
        return;

    m_mappingRangeStart = start;
    m_mappingRangeEnd = end;
    m_mappingRangeLength = end - start;
}

void DoubleSlider::setValue(double value)
{
    value = std::clamp(value, m_mappingRangeStart, m_mappingRangeEnd);

    const int intRangeLength = maximum() - minimum();
    const int intValue = static_cast<int>((value - m_mappingRangeStart) / m_mappingRangeLength * intRangeLength);
    QSlider::setValue(minimum() + intValue);
}

double DoubleSlider::value() const
{
    const int intRangeLength = maximum() - minimum();
    const int intValue = QSlider::value() - minimum();
    return static_cast<double>(intValue) / intRangeLength * m_mappingRangeLength + m_mappingRangeStart;
}

void DoubleSlider::notifyValueChanged(const int value)
{
    const double doubleValue = static_cast<double>(value - minimum()) / (maximum() - minimum()) * m_mappingRangeLength
        + m_mappingRangeStart;
    emit doubleValueChanged(doubleValue);
}