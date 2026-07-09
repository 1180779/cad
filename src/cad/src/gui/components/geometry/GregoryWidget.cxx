//
// Created by Radosław Głasek on 09.07.2026
//

#include "GregoryWidget.hxx"

#include <algorithm>
#include <utility>

#include <QGridLayout>

#include "../../WidgetBuilders.hxx"

using namespace widgets;

// ReSharper disable CppDFAMemoryLeak

GregoryWidget::GregoryWidget(GregoryComponent *gregory, QWidget *parent)
: ComponentWidget(gregory, parent),
  m_gregory(gregory),
  m_lastDivisionsU(gregory->gridDivisionsU()),
  m_lastDivisionsV(gregory->gridDivisionsV()) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    addTitle(layout, "Gregory Patch");
    layout->addWidget(new QLabel(QString("%1 nets (hole edges)").arg(m_gregory->netCount())));

    m_showVectorsCheckbox = addCheckbox(layout, "Show continuity vectors", m_gregory->getShowVectors());
    connect(
        m_showVectorsCheckbox,
        &QCheckBox::toggled,
        this,
        makeBoolToggle(m_gregory, &GregoryComponent::setShowVectors, m_showVectorsCheckbox)
    );

    // master row: sync checkbox + one spinbox driving all nets
    const auto uniformRow = new QHBoxLayout;
    m_uniformCheckbox = new QCheckBox("Sync subdivisions");
    m_uniformSpin = newSpinBox(
        1,
        64,
        m_gregory->netCount() > 0
            ? m_gregory->getGridDivisionsU(0)
            : 4
    );
    m_uniformSpin->setEnabled(false);
    uniformRow->addWidget(m_uniformCheckbox);
    uniformRow->addWidget(m_uniformSpin);
    layout->addLayout(uniformRow);

    const auto grid = new QGridLayout;
    grid->addWidget(new QLabel("U"), 0, 1);
    grid->addWidget(new QLabel("V"), 0, 2);
    for (int net = 0; net < m_gregory->netCount(); ++net) {
        grid->addWidget(new QLabel(QString("Net %1:").arg(net + 1)), net + 1, 0);
        const auto uSpin = newSpinBox(1, 64, m_gregory->getGridDivisionsU(net));
        const auto vSpin = newSpinBox(1, 64, m_gregory->getGridDivisionsV(net));
        grid->addWidget(uSpin, net + 1, 1);
        grid->addWidget(vSpin, net + 1, 2);
        m_divisionsUSpins.push_back(uSpin);
        m_divisionsVSpins.push_back(vSpin);
        const auto onUChanged = [this, net](const int value) {
            subdivisionChanged(&GregoryComponent::setGridDivisionsU, m_lastDivisionsU, m_divisionsUSpins, net, value);
        };
        const auto onVChanged = [this, net](const int value) {
            subdivisionChanged(&GregoryComponent::setGridDivisionsV, m_lastDivisionsV, m_divisionsVSpins, net, value);
        };
        connect(uSpin, &QSpinBox::valueChanged, this, onUChanged);
        connect(vSpin, &QSpinBox::valueChanged, this, onVChanged);
    }
    layout->addLayout(grid);

    connect(m_uniformCheckbox, &QCheckBox::toggled, this, &GregoryWidget::uniformToggled);
    connect(m_uniformSpin, &QSpinBox::valueChanged, this, &GregoryWidget::applyUniform);
}

// ReSharper restore CppDFAMemoryLeak

void GregoryWidget::subdivisionChanged(
    void (GregoryComponent::*setter)(int, int),
    std::vector<int> &lastValues,
    const std::vector<QSpinBox*> &spins,
    const int net,
    const int value
) {
    const int previous = std::exchange(lastValues[net], value);
    pushEdit(
        [this, setter, net, value] {
            (m_gregory->*setter)(net, value);
        },
        [this, setter, net, previous] {
            (m_gregory->*setter)(net, previous);
        },
        spins[net],
        true
    );
    emit propertyChanged();
}

void GregoryWidget::applyUniform(const int value) {
    const auto previousU = m_lastDivisionsU;
    const auto previousV = m_lastDivisionsV;
    std::ranges::fill(m_lastDivisionsU, value);
    std::ranges::fill(m_lastDivisionsV, value);
    for (int net = 0; net < m_gregory->netCount(); ++net) {
        const QSignalBlocker bu(m_divisionsUSpins[net]);
        const QSignalBlocker bv(m_divisionsVSpins[net]);
        m_divisionsUSpins[net]->setValue(value);
        m_divisionsVSpins[net]->setValue(value);
    }
    pushEdit(
        [this, value] {
            for (int net = 0; net < m_gregory->netCount(); ++net) {
                m_gregory->setGridDivisionsU(net, value);
                m_gregory->setGridDivisionsV(net, value);
            }
        },
        [this, previousU, previousV] {
            for (int net = 0; net < m_gregory->netCount(); ++net) {
                m_gregory->setGridDivisionsU(net, previousU[net]);
                m_gregory->setGridDivisionsV(net, previousV[net]);
            }
        },
        m_uniformSpin,
        true
    );
    emit propertyChanged();
}

void GregoryWidget::uniformToggled(const bool checked) {
    m_uniformSpin->setEnabled(checked);
    for (const auto spin : m_divisionsUSpins) {
        spin->setEnabled(!checked);
    }
    for (const auto spin : m_divisionsVSpins) {
        spin->setEnabled(!checked);
    }
    if (checked) {
        applyUniform(m_uniformSpin->value());
    }
}
