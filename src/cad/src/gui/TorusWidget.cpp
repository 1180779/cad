#include "TorusWidget.hpp"
#include <QLabel>

TorusWidget::TorusWidget(TorusGeometry *torus, QWidget *parent) : ComponentWidget(torus, parent), m_torus(torus) {
    const auto layout = new QFormLayout(this);

    setUpMajorRadiusControls(layout);
    setUpMinorRadiusControls(layout);
    setUpMajorSegmentsControls(layout);
    setUpMinorSegmentsControls(layout);

    // route edits through the command stack (coalescing keyed by source spinbox);
    // the model->spinbox connects below keep the UI in sync on undo/redo

    connect(
        m_majorRadius,
        &QDoubleSpinBox::valueChanged,
        this,
        [this](const double v) {
            auto *t = m_torus;
            const auto before = t->getMajorRadius();
            const auto after = static_cast<cadm::cadf>(v);
            pushEdit(
                [t, after] {
                    t->setMajorRadius(after);
                },
                [t, before] {
                    t->setMajorRadius(before);
                },
                m_majorRadius,
                true
            );
        }
    );
    connect(
        m_minorRadius,
        &QDoubleSpinBox::valueChanged,
        this,
        [this](const double v) {
            auto *t = m_torus;
            const auto before = t->getMinorRadius();
            const auto after = static_cast<cadm::cadf>(v);
            pushEdit(
                [t, after] {
                    t->setMinorRadius(after);
                },
                [t, before] {
                    t->setMinorRadius(before);
                },
                m_minorRadius,
                true
            );
        }
    );
    connect(
        m_majorSegments,
        &QSpinBox::valueChanged,
        this,
        [this](const int v) {
            auto *t = m_torus;
            const auto before = t->getMajorSegments();
            const auto after = static_cast<uint32_t>(v);
            pushEdit(
                [t, after] {
                    t->setMajorSegments(after);
                },
                [t, before] {
                    t->setMajorSegments(before);
                },
                m_majorSegments,
                true
            );
        }
    );
    connect(
        m_minorSegments,
        &QSpinBox::valueChanged,
        this,
        [this](const int v) {
            auto *t = m_torus;
            const auto before = t->getMinorSegments();
            const auto after = static_cast<uint32_t>(v);
            pushEdit(
                [t, after] {
                    t->setMinorSegments(after);
                },
                [t, before] {
                    t->setMinorSegments(before);
                },
                m_minorSegments,
                true
            );
        }
    );

    connect(m_torus, &TorusGeometry::majorRadiusChanged, this, &TorusWidget::onMajorRadiusChanged);
    connect(m_torus, &TorusGeometry::minorRadiusChanged, this, &TorusWidget::onMinorRadiusChanged);
    connect(m_torus, &TorusGeometry::majorSegmentsChanged, this, &TorusWidget::onMajorSegmentsChanged);
    connect(m_torus, &TorusGeometry::minorSegmentsChanged, this, &TorusWidget::onMinorSegmentsChanged);
}

void TorusWidget::onMajorRadiusChanged(const double value) const {
    m_majorRadius->setValue(value);
    m_minorRadius->setMaximum(value - s_minorRadiusMin);
    emit
    const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMinorRadiusChanged(const double value) const {
    m_minorRadius->setValue(value);
    emit
    const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMajorSegmentsChanged(const int value) const {
    m_majorSegments->setValue(value);
    emit
    const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::onMinorSegmentsChanged(const int value) const {
    m_minorSegments->setValue(value);
    emit
    const_cast<TorusWidget*>(this)->propertyChanged();
}

void TorusWidget::setUpMajorRadiusControls(QFormLayout * const layout) {
    m_majorRadius = new ModifierDoubleSpinBox();
    m_majorRadius->setMinimum(s_majorRadiusMin);
    m_majorRadius->setMaximum(s_majorRadiusMax);
    m_majorRadius->setSingleStep(s_majorRadiusStep);
    m_majorRadius->setValue(m_torus->getMajorRadius());
    m_majorRadius->setKeyboardTracking(true);
    m_majorRadius->setFixedWidth(s_doubleSpinBoxFixedWidth);
    layout->addRow(new QLabel("Major Radius"), m_majorRadius);
}

void TorusWidget::setUpMinorRadiusControls(QFormLayout * const layout) {
    m_minorRadius = new ModifierDoubleSpinBox();
    m_minorRadius->setMinimum(s_minorRadiusMin);
    m_minorRadius->setMaximum(m_torus->getMajorRadius() - s_minorRadiusMin);
    m_minorRadius->setSingleStep(s_minorRadiusStep);
    m_minorRadius->setValue(m_torus->getMinorRadius());
    m_minorRadius->setKeyboardTracking(true);
    m_minorRadius->setFixedWidth(s_doubleSpinBoxFixedWidth);
    layout->addRow(new QLabel("Minor Radius"), m_minorRadius);
}

void TorusWidget::setUpMajorSegmentsControls(QFormLayout * const layout) {
    m_majorSegments = new ModifierSpinBox();
    m_majorSegments->setMinimum(s_majorSegmentsMin);
    m_majorSegments->setMaximum(s_majorSegmentsMax);
    m_majorSegments->setValue(static_cast<int>(m_torus->getMajorSegments()));
    m_majorSegments->setKeyboardTracking(true);
    m_majorSegments->setFixedWidth(s_doubleSpinBoxFixedWidth);
    layout->addRow(new QLabel("Major Segments"), m_majorSegments);
}

void TorusWidget::setUpMinorSegmentsControls(QFormLayout * const layout) {
    m_minorSegments = new ModifierSpinBox();
    m_minorSegments->setMinimum(s_minorSegmentsMin);
    m_minorSegments->setMaximum(s_minorSegmentsMax);
    m_minorSegments->setValue(static_cast<int>(m_torus->getMinorSegments()));
    m_minorSegments->setKeyboardTracking(true);
    m_minorSegments->setFixedWidth(s_doubleSpinBoxFixedWidth);
    layout->addRow(new QLabel("Minor Segments"), m_minorSegments);
}
