//
// Created by Radosław Głasek on 02.07.2026
//

#include "PatchCreatorDialog.hxx"

#include "../../DialogCard.hxx"
#include "../../WidgetBuilders.hxx"

using namespace widgets;

PatchCreatorDialog::PatchCreatorDialog(const bool c2, QWidget *parent)
: QDialog(parent),
  m_c2(c2) {
    const QString title = c2
                              ? "New Bézier Patch (C2)"
                              : "New Bézier Patch (C0)";
    const auto form = buildDialogCard(this, title);
    m_form = form;

    m_typeCombo = addFormComboBox(form, "Type:", {"Plane", "Cylinder"});
    m_seamCombo = addFormComboBox(form, "Seam direction:", {"V (columns)", "U (rows)"});
    m_countX = addFormSpinBox(form, "Patches along:", 1, 64, 1);
    m_countY = addFormSpinBox(form, "Patches across:", 1, 64, 1);
    m_width = addFormDoubleSpinBox(form, "Width:", 0.1, 1000.0, 5.0);
    m_length = addFormDoubleSpinBox(form, "Length:", 0.1, 1000.0, 5.0);
    m_radius = addFormDoubleSpinBox(form, "Radius:", 0.1, 1000.0, 1.0);
    m_height = addFormDoubleSpinBox(form, "Height:", 0.1, 1000.0, 5.0);

    // uniform field column; counts centered since they're short integers
    m_typeCombo->setFixedWidth(90);
    m_seamCombo->setFixedWidth(90);
    for (QAbstractSpinBox *box : std::initializer_list<QAbstractSpinBox*>{
             m_countX,
             m_countY,
             m_width,
             m_length,
             m_radius,
             m_height
         }) {
        box->setFixedWidth(90);
    }
    m_countX->setAlignment(Qt::AlignCenter);
    m_countY->setAlignment(Qt::AlignCenter);

    m_showNet = new QCheckBox;
    m_showNet->setChecked(true);
    form->addRow("Show control net:", m_showNet);
    connect(m_showNet, &QCheckBox::toggled, this, &PatchCreatorDialog::showNetChanged);

    m_hideScene = new QCheckBox;
    form->addRow("Hide other objects:", m_hideScene);
    connect(m_hideScene, &QCheckBox::toggled, this, &PatchCreatorDialog::hideSceneChanged);

    addDialogButtons(this, form);

    const auto emitParams = [this] {
        emit paramsChanged(params());
    };
    const auto comboAction = [this, emitParams] {
        updateForType();
        emitParams();
    };
    connect(m_typeCombo, &QComboBox::currentIndexChanged, this, comboAction);
    connect(m_seamCombo, &QComboBox::currentIndexChanged, this, comboAction);
    connect(m_countX, &QSpinBox::valueChanged, this, emitParams);
    connect(m_countY, &QSpinBox::valueChanged, this, emitParams);
    connect(m_width, &QDoubleSpinBox::valueChanged, this, emitParams);
    connect(m_length, &QDoubleSpinBox::valueChanged, this, emitParams);
    connect(m_radius, &QDoubleSpinBox::valueChanged, this, emitParams);
    connect(m_height, &QDoubleSpinBox::valueChanged, this, emitParams);

    updateForType();
    // initial state for the live preview once the caller has connected
    QMetaObject::invokeMethod(
        this,
        [this, emitParams] {
            emit showNetChanged(m_showNet->isChecked());
            emit hideSceneChanged(m_hideScene->isChecked());
            emitParams();
        },
        Qt::QueuedConnection
    );
}

WrapDirection PatchCreatorDialog::seam() const {
    return m_seamCombo->currentIndex() == 1
               ? WrapDirection::u
               : WrapDirection::v;
}

bool PatchCreatorDialog::wrapsAlongRows() const {
    return seam() == WrapDirection::u;
}

void PatchCreatorDialog::setFieldLabel(QWidget *field, const QString &text) const {
    if (const auto label = qobject_cast<QLabel*>(m_form->labelForField(field))) {
        label->setText(text);
    }
}

void PatchCreatorDialog::updateForType() const {
    const bool isCylinder = m_typeCombo->currentIndex() == 1;

    const bool aroundIsY = isCylinder && wrapsAlongRows();
    m_countX->setMinimum(
        isCylinder && !aroundIsY
            ? 3
            : 1
    );
    m_countY->setMinimum(
        aroundIsY
            ? 3
            : 1
    );

    setFieldLabel(
        m_countX,
        isCylinder && !aroundIsY
            ? "Patches around:"
            : "Patches along:"
    );
    setFieldLabel(
        m_countY,
        aroundIsY
            ? "Patches around:"
            : "Patches across:"
    );

    m_seamCombo->setEnabled(isCylinder);
    m_width->setEnabled(!isCylinder);
    m_length->setEnabled(!isCylinder);
    m_radius->setEnabled(isCylinder);
    m_height->setEnabled(isCylinder);
}

patchgen::PatchCreateParams PatchCreatorDialog::params() const {
    patchgen::PatchCreateParams p;
    p.type = m_c2
                 ? patchgen::PatchCreateParams::Type::c2
                 : patchgen::PatchCreateParams::Type::c0;
    p.patchCountX = m_countX->value();
    p.patchCountY = m_countY->value();
    if (const bool isCylinder = m_typeCombo->currentIndex() == 1;
        isCylinder) {
        p.dimensions = patchgen::CylinderDimensions{
            .radius = static_cast<cadm::cadf>(m_radius->value()),
            .height = static_cast<cadm::cadf>(m_height->value()),
            .seam = seam(),
        };
    }
    else {
        p.dimensions = patchgen::PlaneExtents{
            .width = static_cast<cadm::cadf>(m_width->value()),
            .length = static_cast<cadm::cadf>(m_length->value()),
        };
    }
    return p;
}
