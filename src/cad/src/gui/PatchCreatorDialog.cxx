//
// Created by Radosław Głasek on 02.07.2026
//

#include "PatchCreatorDialog.hxx"

#include <QDialogButtonBox>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QVBoxLayout>

#include "CadTitleBar.hpp"
#include "Theme.hpp"
#include "WidgetBuilders.hxx"

using namespace widgets;

PatchCreatorDialog::PatchCreatorDialog(const bool c2, QWidget *parent) : QDialog(parent), m_c2(c2) {
    const QString title = c2
                              ? "New Bézier Patch (C2)"
                              : "New Bézier Patch (C0)";
    setWindowTitle(title);

    // frameless, drag comes from DialogTitleBar; deliberately non-modal so the
    // main window stays interactive (camera navigation) for the live preview
    setWindowFlag(Qt::FramelessWindowHint);
    // translucent window so the rounded card corners and drop shadow render
    setAttribute(Qt::WA_TranslucentBackground);

    setStyleSheet(theme::dialogCardStyle(theme::active()));

    // ReSharper disable once CppDFAMemoryLeak
    const auto shell = new QVBoxLayout(this);
    shell->setContentsMargins(12, 12, 12, 12);

    // ReSharper disable once CppDFAMemoryLeak
    const auto card = new QWidget(this);
    card->setObjectName("dialogCard");
    // ReSharper disable once CppDFAMemoryLeak
    const auto shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 90));
    card->setGraphicsEffect(shadow);
    shell->addWidget(card);

    // ReSharper disable once CppDFAMemoryLeak
    const auto outer = new QVBoxLayout(card);
    outer->setContentsMargins(1, 1, 1, 1);
    outer->setSpacing(0);
    // ReSharper disable once CppDFAMemoryLeak
    const auto titleBar = new DialogTitleBar(title);
    titleBar->setAutoFillBackground(false); // QSS above paints the tinted strip
    titleBar->setAttribute(Qt::WA_StyledBackground); // plain QWidget needs this for QSS backgrounds
    outer->addWidget(titleBar);

    // ReSharper disable once CppDFAMemoryLeak
    const auto form = new QFormLayout;
    form->setContentsMargins(14, 12, 14, 14);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);
    outer->addLayout(form);

    m_typeCombo = addFormComboBox(form, "Type:", {"Plane", "Cylinder"});
    m_countX = addFormSpinBox(form, "Patches along (around):", 1, 64, 1);
    m_countY = addFormSpinBox(form, "Patches across:", 1, 64, 1);
    m_width = addFormDoubleSpinBox(form, "Width:", 0.1, 1000.0, 5.0);
    m_length = addFormDoubleSpinBox(form, "Length:", 0.1, 1000.0, 5.0);
    m_radius = addFormDoubleSpinBox(form, "Radius:", 0.1, 1000.0, 1.0);
    m_height = addFormDoubleSpinBox(form, "Height:", 0.1, 1000.0, 5.0);

    // uniform field column; counts centered since they're short integers
    m_typeCombo->setFixedWidth(90);
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

    // ReSharper disable once CppDFAMemoryLeak
    m_showNet = new QCheckBox;
    m_showNet->setChecked(true);
    form->addRow("Show control net:", m_showNet);
    connect(m_showNet, &QCheckBox::toggled, this, &PatchCreatorDialog::showNetChanged);

    // ReSharper disable once CppDFAMemoryLeak
    m_hideScene = new QCheckBox;
    form->addRow("Hide other objects:", m_hideScene);
    connect(m_hideScene, &QCheckBox::toggled, this, &PatchCreatorDialog::hideSceneChanged);

    // ReSharper disable once CppDFAMemoryLeak
    const auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setObjectName("okButton");
    buttons->button(QDialogButtonBox::Cancel)->setObjectName("cancelButton");
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const auto emitParams = [this] {
        emit paramsChanged(params());
    };
    connect(
        m_typeCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this, emitParams] {
            updateForType();
            emitParams();
        }
    );
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
    setSizeGripEnabled(false);
    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void PatchCreatorDialog::updateForType() const {
    const bool isCylinder = m_typeCombo->currentIndex() == 1;
    m_countX->setMinimum(
        isCylinder
            ? 3
            : 1
    );

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
