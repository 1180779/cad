//
// Created on 6/21/26.
//

#include "VirtualPointPropertiesWidget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <common/CoordinateSpinBox.hpp>

VirtualPointPropertiesWidget::VirtualPointPropertiesWidget(QWidget *parent) : QWidget(parent) {
    // ReSharper disable once CppDFAMemoryLeak
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_caption = new QLabel("Selected point:");
    layout->addWidget(m_caption);

    // ReSharper disable once CppDFAMemoryLeak
    const auto row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    coordSpinBox::setUpRows(row, m_x, m_y, m_z);
    layout->addLayout(row);

    const auto onChanged = [this] {
        if (m_refreshing) {
            return;
        }
        emit coordinateEdited(currentValue());
    };
    connect(m_x, &QDoubleSpinBox::valueChanged, this, onChanged);
    connect(m_y, &QDoubleSpinBox::valueChanged, this, onChanged);
    connect(m_z, &QDoubleSpinBox::valueChanged, this, onChanged);

    setActive(false);
}

void VirtualPointPropertiesWidget::setCaption(const QString &text) const {
    m_caption->setText(text);
}

void VirtualPointPropertiesWidget::setPosition(const cadm::Vec3 pos) {
    m_refreshing = true;
    m_x->setValue(pos.x);
    m_y->setValue(pos.y);
    m_z->setValue(pos.z);
    m_refreshing = false;
}

void VirtualPointPropertiesWidget::setActive(const bool active) const {
    m_caption->setEnabled(active);
    m_x->setEnabled(active);
    m_y->setEnabled(active);
    m_z->setEnabled(active);
}

cadm::Vec3 VirtualPointPropertiesWidget::currentValue() const {
    return {
        static_cast<cadm::cadf>(m_x->value()),
        static_cast<cadm::cadf>(m_y->value()),
        static_cast<cadm::cadf>(m_z->value())
    };
}
