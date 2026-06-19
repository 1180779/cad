#include "TransformWidget.hpp"
#include <QFormLayout>
#include <QLabel>
#include <numbers>

TransformWidget::TransformWidget(TransformComponent *transform, QWidget *parent) : ComponentWidget(transform, parent),
    m_transform(transform) {
    const auto layout = new QFormLayout(this);

    const auto addAxisRow = [&](
        const QString &label,
        ModifierDoubleSpinBox *x,
        ModifierDoubleSpinBox *y,
        ModifierDoubleSpinBox *z
    ) {
        const auto row = new QHBoxLayout();
        row->addWidget(x);
        row->addWidget(y);
        row->addWidget(z);
        layout->addRow(new QLabel(label), row);
    };

    /// setUpTranslationControls
    const auto translation = m_transform->getTranslation();
    m_translationX = makeAxisSpin(s_translationStep, s_translationMin, s_translationMax, false, translation.x);
    m_translationY = makeAxisSpin(s_translationStep, s_translationMin, s_translationMax, false, translation.y);
    m_translationZ = makeAxisSpin(s_translationStep, s_translationMin, s_translationMax, false, translation.z);
    connect(m_translationX, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onTranslationXChanged);
    connect(m_translationY, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onTranslationYChanged);
    connect(m_translationZ, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onTranslationZChanged);
    addAxisRow("Translation", m_translationX, m_translationY, m_translationZ);

    /// setUpScaleControls
    const auto scale = m_transform->getScale();
    m_scaleX = makeAxisSpin(s_scaleStep, s_scaleMin, s_scaleMax, false, scale.x);
    m_scaleY = makeAxisSpin(s_scaleStep, s_scaleMin, s_scaleMax, false, scale.y);
    m_scaleZ = makeAxisSpin(s_scaleStep, s_scaleMin, s_scaleMax, false, scale.z);
    connect(m_scaleX, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onScaleXChanged);
    connect(m_scaleY, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onScaleYChanged);
    connect(m_scaleZ, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onScaleZChanged);
    addAxisRow("Scale", m_scaleX, m_scaleY, m_scaleZ);

    /// setUpRotationControls
    constexpr double radToDeg = 180.0 / std::numbers::pi;
    const auto rotation = m_transform->getRotation();
    m_rotationX = makeAxisSpin(s_angleStep, s_angleMin, s_angleMax, true, rotation.x * radToDeg);
    m_rotationY = makeAxisSpin(s_angleStep, s_angleMin, s_angleMax, true, rotation.y * radToDeg);
    m_rotationZ = makeAxisSpin(s_angleStep, s_angleMin, s_angleMax, true, rotation.z * radToDeg);
    connect(m_rotationX, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onRotationXChanged);
    connect(m_rotationY, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onRotationYChanged);
    connect(m_rotationZ, &QDoubleSpinBox::valueChanged, this, &TransformWidget::onRotationZChanged);
    addAxisRow("Rotation", m_rotationX, m_rotationY, m_rotationZ);

    // connect
    connect(
        m_transform,
        &TransformComponent::translationXChanged,
        this,
        [this](const double v) {
            m_translationX->blockSignals(true);
            m_translationX->setValue(v);
            m_translationX->blockSignals(false);
        }
    );
    connect(
        m_transform,
        &TransformComponent::translationYChanged,
        this,
        [this](const double v) {
            m_translationY->blockSignals(true);
            m_translationY->setValue(v);
            m_translationY->blockSignals(false);
        }
    );
    connect(
        m_transform,
        &TransformComponent::translationZChanged,
        this,
        [this](const double v) {
            m_translationZ->blockSignals(true);
            m_translationZ->setValue(v);
            m_translationZ->blockSignals(false);
        }
    );

    connect(
        m_transform,
        &TransformComponent::scaleXChanged,
        this,
        [this](const double v) {
            m_scaleX->blockSignals(true);
            m_scaleX->setValue(v);
            m_scaleX->blockSignals(false);
        }
    );
    connect(
        m_transform,
        &TransformComponent::scaleYChanged,
        this,
        [this](const double v) {
            m_scaleY->blockSignals(true);
            m_scaleY->setValue(v);
            m_scaleY->blockSignals(false);
        }
    );
    connect(
        m_transform,
        &TransformComponent::scaleZChanged,
        this,
        [this](const double v) {
            m_scaleZ->blockSignals(true);
            m_scaleZ->setValue(v);
            m_scaleZ->blockSignals(false);
        }
    );

    connect(
        m_transform,
        &TransformComponent::rotationXChanged,
        this,
        [this](const double v) {
            m_rotationX->blockSignals(true);
            m_rotationX->setValue(v * 180.0 / std::numbers::pi);
            m_rotationX->blockSignals(false);
        }
    );
    connect(
        m_transform,
        &TransformComponent::rotationYChanged,
        this,
        [this](const double v) {
            m_rotationY->blockSignals(true);
            m_rotationY->setValue(v * 180.0 / std::numbers::pi);
            m_rotationY->blockSignals(false);
        }
    );
    connect(
        m_transform,
        &TransformComponent::rotationZChanged,
        this,
        [this](const double v) {
            m_rotationZ->blockSignals(true);
            m_rotationZ->setValue(v * 180.0 / std::numbers::pi);
            m_rotationZ->blockSignals(false);
        }
    );
}

ModifierDoubleSpinBox* TransformWidget::makeAxisSpin(
    const double step,
    const double min,
    const double max,
    const bool wrapping,
    const double value
) {
    const auto spin = new ModifierDoubleSpinBox();
    spin->setSingleStep(step);
    spin->setRange(min, max);
    spin->setWrapping(wrapping);
    spin->setValue(value);
    spin->setKeyboardTracking(true);
    spin->setFixedWidth(s_doubleSpinBoxFixedWidth);
    return spin;
}

void TransformWidget::onTranslationXChanged(const double value) const {
    const auto before = m_transform->getTranslation();
    auto after = before;
    after.x = static_cast<cadm::cadf>(value);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setTranslation(after);
        },
        [t, before] {
            t->setTranslation(before);
        },
        m_translationX,
        true
    );
}

void TransformWidget::onTranslationYChanged(const double value) const {
    const auto before = m_transform->getTranslation();
    auto after = before;
    after.y = static_cast<cadm::cadf>(value);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setTranslation(after);
        },
        [t, before] {
            t->setTranslation(before);
        },
        m_translationY,
        true
    );
}

void TransformWidget::onTranslationZChanged(const double value) const {
    const auto before = m_transform->getTranslation();
    auto after = before;
    after.z = static_cast<cadm::cadf>(value);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setTranslation(after);
        },
        [t, before] {
            t->setTranslation(before);
        },
        m_translationZ,
        true
    );
}

void TransformWidget::onScaleXChanged(const double value) const {
    const auto before = m_transform->getScale();
    auto after = before;
    after.x = static_cast<cadm::cadf>(value);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setScale(after);
        },
        [t, before] {
            t->setScale(before);
        },
        m_scaleX,
        true
    );
}

void TransformWidget::onScaleYChanged(const double value) const {
    const auto before = m_transform->getScale();
    auto after = before;
    after.y = static_cast<cadm::cadf>(value);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setScale(after);
        },
        [t, before] {
            t->setScale(before);
        },
        m_scaleY,
        true
    );
}

void TransformWidget::onScaleZChanged(const double value) const {
    const auto before = m_transform->getScale();
    auto after = before;
    after.z = static_cast<cadm::cadf>(value);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setScale(after);
        },
        [t, before] {
            t->setScale(before);
        },
        m_scaleZ,
        true
    );
}

void TransformWidget::onRotationXChanged(const double value) const {
    const auto before = m_transform->getRotation();
    auto after = before;
    after.x = static_cast<cadm::cadf>(value * std::numbers::pi / 180);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setRotation(after);
        },
        [t, before] {
            t->setRotation(before);
        },
        m_rotationX,
        true
    );
}

void TransformWidget::onRotationYChanged(const double value) const {
    const auto before = m_transform->getRotation();
    auto after = before;
    after.y = static_cast<cadm::cadf>(value * std::numbers::pi / 180);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setRotation(after);
        },
        [t, before] {
            t->setRotation(before);
        },
        m_rotationY,
        true
    );
}

void TransformWidget::onRotationZChanged(const double value) const {
    const auto before = m_transform->getRotation();
    auto after = before;
    after.z = static_cast<cadm::cadf>(value * std::numbers::pi / 180);
    auto *t = m_transform;
    const_cast<TransformWidget*>(this)->pushEdit(
        [t, after] {
            t->setRotation(after);
        },
        [t, before] {
            t->setRotation(before);
        },
        m_rotationZ,
        true
    );
}
