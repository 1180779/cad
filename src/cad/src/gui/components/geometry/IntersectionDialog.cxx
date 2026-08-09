//
// Created by Radosław Głasek on 02.08.2026
//

#include "IntersectionDialog.hxx"

#include "../../DialogCard.hxx"
#include "../../WidgetBuilders.hxx"

using namespace widgets;

IntersectionDialog::IntersectionDialog(const bool cursorAvailable, QWidget *parent)
: QDialog(parent) {
    const auto form = buildDialogCard(this, "Intersect Surfaces");

    m_step = addFormDoubleSpinBox(form, "Step:", 0.0001, 10.0, 0.01);
    m_step->setDecimals(4);
    m_step->setSingleStep(0.005);
    m_maxPoints = addFormSpinBox(form, "Max points:", 10, 100000, 2000);
    m_step->setFixedWidth(90);
    m_maxPoints->setFixedWidth(90);

    m_useCursor = new QCheckBox;
    m_useCursor->setEnabled(cursorAvailable);
    m_useCursor->setToolTip(
        cursorAvailable
            ? "Trace only the branch nearest the 3D cursor"
            : "No 3D cursor in the scene"
    );
    form->addRow("Start from 3D cursor:", m_useCursor);

    addDialogButtons(this, form);
}

IntersectionParams IntersectionDialog::params() const {
    return {
        .step = static_cast<cadm::cadf>(m_step->value()),
        .maxPoints = m_maxPoints->value(),
        .useCursor = m_useCursor->isChecked(),
    };
}
