//
// Created by Radosław Głasek on 02.07.2026
//

#include "PatchWidget.hxx"

#include "WidgetBuilders.hxx"

using namespace widgets;

namespace {
    QString summaryText(const PatchComponent *patch) {
        return QString("%1 x %2 patches, %3 control points (locked)")
               .arg(patch->getPatchCountX())
               .arg(patch->getPatchCountY())
               .arg(static_cast<int>(patch->getControlPoints().size()));
    }
}

PatchWidget::PatchWidget(PatchComponent *patch, const QString &title, QWidget *parent) : ComponentWidget(patch, parent),
    m_patch(patch),
    m_lastDivisionsU(patch->getGridDivisionsU()),
    m_lastDivisionsV(patch->getGridDivisionsV()) {
    // ReSharper disable once CppDFAMemoryLeak
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    addTitle(layout, title);
    layout->addWidget(new QLabel(summaryText(m_patch)));

    m_showNetCheckbox = addCheckbox(layout, "Show control net", m_patch->getShowNet());
    m_divisionsUSpin = addSpinBox(layout, "Surface subdivisions (U):", 1, 64, m_patch->getGridDivisionsU());
    m_divisionsVSpin = addSpinBox(layout, "Surface subdivisions (V):", 1, 64, m_patch->getGridDivisionsV());

    connect(
        m_showNetCheckbox,
        &QCheckBox::toggled,
        this,
        makeBoolToggle(m_patch, &PatchComponent::setShowNet, m_showNetCheckbox)
    );
    connect(m_divisionsUSpin, &QSpinBox::valueChanged, this, &PatchWidget::subdivisionUChanged);
    connect(m_divisionsVSpin, &QSpinBox::valueChanged, this, &PatchWidget::subdivisionVChanged);
}

void PatchWidget::subdivisionUChanged(const int value) {
    const int previous = m_lastDivisionsU;
    m_lastDivisionsU = value;
    pushEdit(
        [this, value] {
            m_patch->setGridDivisionsU(value);
        },
        [this, previous] {
            m_patch->setGridDivisionsU(previous);
        },
        m_divisionsUSpin,
        true
    );
    emit propertyChanged();
}

void PatchWidget::subdivisionVChanged(const int value) {
    const int previous = m_lastDivisionsV;
    m_lastDivisionsV = value;
    pushEdit(
        [this, value] {
            m_patch->setGridDivisionsV(value);
        },
        [this, previous] {
            m_patch->setGridDivisionsV(previous);
        },
        m_divisionsVSpin,
        true
    );
    emit propertyChanged();
}
