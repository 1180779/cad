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
    m_lastDivisions(patch->getGridDivisions()) {
    // ReSharper disable once CppDFAMemoryLeak
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    addTitle(layout, title);
    layout->addWidget(new QLabel(summaryText(m_patch)));

    m_showNetCheckbox = addCheckbox(layout, "Show control net", m_patch->getShowNet());
    m_divisionsSpin = addSpinBox(layout, "Surface subdivisions:", 1, 64, m_patch->getGridDivisions());

    connect(
        m_showNetCheckbox,
        &QCheckBox::toggled,
        this,
        makeBoolToggle(m_patch, &PatchComponent::setShowNet, m_showNetCheckbox)
    );
    connect(m_divisionsSpin, &QSpinBox::valueChanged, this, &PatchWidget::subdivisionChanged);
}

void PatchWidget::subdivisionChanged(const int value) {
    const int previous = m_lastDivisions;
    m_lastDivisions = value;
    pushEdit(
        [this, value] {
            m_patch->setGridDivisions(value);
        },
        [this, previous] {
            m_patch->setGridDivisions(previous);
        },
        m_divisionsSpin,
        true
    );
    emit propertyChanged();
}
