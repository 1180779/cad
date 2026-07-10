//
// Created by Radosław Głasek on 02.07.2026
//

#include "PatchWidget.hxx"

#include <QGridLayout>

#include "Scene.hpp"
#include "../../WidgetBuilders.hxx"

using namespace widgets;

namespace {
    QString summaryText(const PatchComponent *patch) {
        return QString("%1 x %2 patches, %3 control points (locked)")
               .arg(patch->getPatchCountX())
               .arg(patch->getPatchCountY())
               .arg(static_cast<int>(patch->getControlPoints().size()));
    }
}

// ReSharper disable CppDFAMemoryLeak

PatchWidget::PatchWidget(PatchComponent *patch, const QString &title, QWidget *parent)
: ComponentWidget(patch, parent),
  m_patch(patch),
  m_lastDivisionsU(patch->getGridDivisionsU()),
  m_lastDivisionsV(patch->getGridDivisionsV()) {
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

    addPatchSelectionGrid(layout);
}

void PatchWidget::addPatchSelectionGrid(QVBoxLayout *layout) {
    layout->addWidget(new QLabel("Single patches (u 󰜴, v 󰜮):"));
    const auto grid = new QGridLayout();
    grid->setSpacing(2);
    for (int px = 0; px < m_patch->getPatchCountX(); ++px) {
        const auto label = new QLabel(QString("u%1").arg(px));
        label->setAlignment(Qt::AlignCenter);
        grid->addWidget(label, 0, px + 1);
    }
    for (int py = 0; py < m_patch->getPatchCountY(); ++py) {
        const auto label = new QLabel(QString("v%1").arg(py));
        label->setAlignment(Qt::AlignCenter);
        grid->addWidget(label, py + 1, 0);
    }
    const auto buttonToggled = [this](int index) {
        return [this, index](const bool checked) {
            m_patch->setPatchSelected(index, checked);
            emit propertyChanged();
        };
    };
    for (int py = 0; py < m_patch->getPatchCountY(); ++py) {
        for (int px = 0; px < m_patch->getPatchCountX(); ++px) {
            const int index = py * m_patch->getPatchCountX() + px;
            const auto button = new QPushButton();
            button->setCheckable(true);
            button->setChecked(m_patch->isPatchSelected(index));
            button->setFixedSize(28, 28);
            button->setStyleSheet("QPushButton:checked { background-color: #cc7a1f; }");
            connect(button, &QPushButton::toggled, this, buttonToggled(index));
            m_patchButtons.push_back(button);
            grid->addWidget(button, py + 1, px + 1);
        }
    }
    grid->setColumnStretch(m_patch->getPatchCountX() + 1, 1);
    layout->addLayout(grid);

    const auto setCheckedSlot = [this](bool checked) {
        return [this, checked] {
            for (const auto button : m_patchButtons) {
                button->setChecked(checked);
            }
            emit propertyChanged();
        };
    };
    const auto clearButton = new QPushButton("Clear patch selection");
    const auto selectAllButton = new QPushButton("Select all patches");
    connect(clearButton, &QPushButton::clicked, this, setCheckedSlot(false));
    connect(selectAllButton, &QPushButton::clicked, this, setCheckedSlot(true));
    layout->addWidget(clearButton);
    layout->addWidget(selectAllButton);

    const auto selectPointsButton = new QPushButton("Select patch points");
    connect(selectPointsButton, &QPushButton::clicked, this, &PatchWidget::selectPatchPoints);
    layout->addWidget(selectPointsButton);
}

// ReSharper restore CppDFAMemoryLeak

void PatchWidget::selectPatchPoints() {
    if (!m_scene || m_patch->getSelectedPatches().empty()) {
        return;
    }
    std::set<PointHandle> handles;
    for (const int q : m_patch->getSelectedPatches()) {
        const auto view = m_patch->singlePatch(q % m_patch->getPatchCountX(), q / m_patch->getPatchCountX());
        const auto viewH = view.handles();
        handles.insert(viewH.cbegin(), viewH.cend());
    }
    m_scene->clearSelection();
    for (const PointHandle h : handles) {
        if (const auto e = m_scene->getEntityByPointHandle(h)) {
            m_scene->setSelected(e.value(), true);
        }
    }
    m_scene->syncPointSelectionToRegistry();
    emit propertyChanged();
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
