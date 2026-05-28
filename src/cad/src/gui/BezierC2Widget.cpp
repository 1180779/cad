//
// Created on 5/5/26.
//

#include "BezierC2Widget.hpp"

#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

BezierC2Widget::BezierC2Widget(BezierC2Component *bezier, Scene *scene, QWidget *parent) : ComponentWidget(
        bezier,
        parent
    ),
    m_bezier(bezier),
    m_scene(scene) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    const auto titleLabel = new QLabel("Bezier C2 Curve");
    QFont f = titleLabel->font();
    f.setBold(true);
    titleLabel->setFont(f);
    layout->addWidget(titleLabel);

    m_showDeBoorPolygonCheckbox = new QCheckBox("Show de Boor polygon");
    m_showDeBoorPolygonCheckbox->setChecked(bezier->getShowDeBoorPolygon());
    layout->addWidget(m_showDeBoorPolygonCheckbox);

    m_showBernsteinPolygonCheckbox = new QCheckBox("Show Bernstein polygon");
    m_showBernsteinPolygonCheckbox->setChecked(bezier->getShowBernsteinPolygon());
    layout->addWidget(m_showBernsteinPolygonCheckbox);

    m_uniformCheckbox = new QCheckBox("Uniform parametrization");
    m_uniformCheckbox->setChecked(bezier->getParametrizationMode() == ParametrizationMode::uniform);
    layout->addWidget(m_uniformCheckbox);

    // De Boor list
    layout->addWidget(new QLabel("De Boor points:"));
    m_deBoorList = new QListWidget;
    m_deBoorList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_deBoorList->setMaximumHeight(120);
    layout->addWidget(m_deBoorList);

    m_removeButton = new QPushButton("Remove selected from curve");
    layout->addWidget(m_removeButton);

    auto *sep1 = new QFrame;
    sep1->setFrameShape(QFrame::HLine);
    layout->addWidget(sep1);

    // Bernstein list
    layout->addWidget(new QLabel("Bernstein points:"));
    m_bernsteinList = new QListWidget;
    m_bernsteinList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_bernsteinList->setMaximumHeight(120);
    layout->addWidget(m_bernsteinList);

    auto *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    layout->addWidget(sep2);

    // shared spinboxes for selected point
    m_selectedLabel = new QLabel("Selected point:");
    layout->addWidget(m_selectedLabel);

    const auto spinForm = new QFormLayout;
    spinForm->setContentsMargins(0, 0, 0, 0);

    auto makeSpinbox = [&]() {
        auto *s = new QDoubleSpinBox;
        s->setRange(-1e6, 1e6);
        s->setDecimals(4);
        s->setSingleStep(0.1);
        return s;
    };

    m_xSpin = makeSpinbox();
    m_ySpin = makeSpinbox();
    m_zSpin = makeSpinbox();
    spinForm->addRow("X:", m_xSpin);
    spinForm->addRow("Y:", m_ySpin);
    spinForm->addRow("Z:", m_zSpin);
    layout->addLayout(spinForm);

    setSpinboxesEnabled(false);
    m_selectedLabel->setEnabled(false);

    refreshList();
    m_bernsteinList->setEnabled(true);

    // connections
    connect(
        m_showDeBoorPolygonCheckbox,
        &QCheckBox::toggled,
        this,
        [this](const bool checked) {
            m_bezier->setShowDeBoorPolygon(checked);
            emit propertyChanged();
        }
    );

    connect(
        m_showBernsteinPolygonCheckbox,
        &QCheckBox::toggled,
        this,
        [this](const bool checked) {
            m_bezier->setShowBernsteinPolygon(checked);
            emit propertyChanged();
        }
    );

    connect(
        m_uniformCheckbox,
        &QCheckBox::toggled,
        this,
        [this](const bool checked) {
            m_bezier->setParametrizationMode(
                checked
                    ? ParametrizationMode::uniform
                    : ParametrizationMode::chordLength
            );
            emit propertyChanged();
        }
    );

    connect(
        m_removeButton,
        &QPushButton::clicked,
        this,
        [this] {
            const int row = m_deBoorList->currentRow();
            if (row < 0) { return; }
            if (const PointHandle removing = m_bezier->getDeBoorPoints()[row];
                m_selectedKind == SelectedPointKind::DeBoor && m_selectedDeBoor == removing) {
                m_selectedKind = SelectedPointKind::None;
                m_selectedDeBoor = InvalidPointHandle;
                setSpinboxesEnabled(false);
                m_selectedLabel->setEnabled(false);
            }
            m_bezier->removeControlPointAt(row);
            refreshList();
            emit propertyChanged();
        }
    );

    connect(
        m_deBoorList,
        &QListWidget::itemSelectionChanged,
        this,
        [this] {
            if (!m_scene) { return; }

            QList<Entity*> selected;
            for (const auto *item : m_deBoorList->selectedItems()) {
                const auto h = item->data(Qt::UserRole).value<PointHandle>();
                if (const auto opt = m_scene->getEntityByPointHandle(h)) { selected.append(opt.value()); }
            }
            emit pointSelectionChanged(selected);

            // update spinboxes for the single current item
            const auto *cur = m_deBoorList->currentItem();
            if (!cur || !cur->isSelected()) {
                if (m_selectedKind == SelectedPointKind::DeBoor) {
                    m_selectedKind = SelectedPointKind::None;
                    setSpinboxesEnabled(false);
                    m_selectedLabel->setEnabled(false);
                }
                return;
            }

            // clear bernstein selection
            {
                const QSignalBlocker b(m_bernsteinList);
                m_bernsteinList->clearSelection();
            }

            m_selectedKind = SelectedPointKind::DeBoor;
            m_selectedDeBoor = cur->data(Qt::UserRole).value<PointHandle>();
            m_selectedBernstein = -1;
            m_selectedLabel->setEnabled(true);
            updateSpinboxesForDeBoor(m_selectedDeBoor);
            setSpinboxesEnabled(true);
        }
    );

    connect(
        m_bernsteinList,
        &QListWidget::itemSelectionChanged,
        this,
        [this] {
            const auto *cur = m_bernsteinList->currentItem();
            if (!cur || !cur->isSelected()) {
                if (m_selectedKind == SelectedPointKind::Bernstein) {
                    m_selectedKind = SelectedPointKind::None;
                    setSpinboxesEnabled(false);
                    m_selectedLabel->setEnabled(false);
                }
                return;
            }

            // clear de Boor selection
            {
                const QSignalBlocker b(m_deBoorList);
                m_deBoorList->clearSelection();
            }

            m_selectedKind = SelectedPointKind::Bernstein;
            m_selectedDeBoor = InvalidPointHandle;
            m_selectedBernstein = cur->data(Qt::UserRole).toInt();
            m_selectedLabel->setEnabled(true);
            updateSpinboxesForBernstein(m_selectedBernstein);
            setSpinboxesEnabled(true);
        }
    );

    // spinbox edits
    auto onSpinChanged = [this] {
        if (m_spinboxRefreshing) { return; }
        const cadm::vec3 newPos{
            static_cast<float>(m_xSpin->value()),
            static_cast<float>(m_ySpin->value()),
            static_cast<float>(m_zSpin->value())
        };
        if (m_selectedKind == SelectedPointKind::DeBoor &&
            m_selectedDeBoor != InvalidPointHandle) {
            m_scene->getPointRegistry().setPosition(m_selectedDeBoor, newPos);
            emit propertyChanged();
        }
        else if (m_selectedKind == SelectedPointKind::Bernstein &&
            m_selectedBernstein >= 0) {
            m_bezier->setBernsteinPosition(m_selectedBernstein, newPos);
            m_scene->getPointRegistry().syncToGpu();
            emit propertyChanged();
        }
    };
    connect(m_xSpin, &QDoubleSpinBox::valueChanged, this, onSpinChanged);
    connect(m_ySpin, &QDoubleSpinBox::valueChanged, this, onSpinChanged);
    connect(m_zSpin, &QDoubleSpinBox::valueChanged, this, onSpinChanged);
}

void BezierC2Widget::refreshList() {
    m_deBoorList->clear();
    m_deBoorItemMap.clear();
    const auto &deBoorPts = m_bezier->getDeBoorPoints();
    for (int i = 0; i < static_cast<int>(deBoorPts.size()); ++i) {
        const PointHandle h = deBoorPts[i];
        QString name = QString("Point #%1").arg(h);
        if (m_scene) {
            if (const auto entityOpt = m_scene->getEntityByPointHandle(h)) {
                name = QString::fromStdString(entityOpt.value()->getName());
            }
        }
        const auto item = new QListWidgetItem(QString("[%1] %2").arg(i).arg(name));
        item->setData(Qt::UserRole, QVariant::fromValue(h));
        m_deBoorList->addItem(item);
        m_deBoorItemMap[h] = item;
    }

    refreshBernsteinList();
}

void BezierC2Widget::refreshBernsteinList() const {
    m_bernsteinList->clear();
    const auto &positions = m_bezier->getBernsteinPositions();
    const int count = static_cast<int>(positions.size());
    for (int i = 0; i < count; ++i) {
        const int seg = i / 4;
        const int local = i % 4;
        const auto item = new QListWidgetItem(
            QString("b%1 [seg %2]").arg(local).arg(seg)
        );
        item->setData(Qt::UserRole, i);
        m_bernsteinList->addItem(item);
    }

    // re-select previously selected index if still valid
    if (m_selectedKind == SelectedPointKind::Bernstein &&
        m_selectedBernstein >= 0 && m_selectedBernstein < count) {
        const QSignalBlocker b(m_bernsteinList);
        m_bernsteinList->setCurrentRow(m_selectedBernstein);
    }
}

void BezierC2Widget::syncSelection() {
    if (!m_scene) { return; }
    for (const auto &[h, item] : m_deBoorItemMap) {
        if (const auto opt = m_scene->getEntityByPointHandle(h)) { item->setSelected(opt.value()->isSelected()); }
    }
}

void BezierC2Widget::refresh() {
    refreshBernsteinList();

    if (m_selectedKind == SelectedPointKind::DeBoor &&
        m_selectedDeBoor != InvalidPointHandle) { updateSpinboxesForDeBoor(m_selectedDeBoor); }
    else if (m_selectedKind == SelectedPointKind::Bernstein &&
        m_selectedBernstein >= 0 &&
        m_selectedBernstein < static_cast<int>(m_bezier->getBernsteinPositions().size())) {
        updateSpinboxesForBernstein(m_selectedBernstein);
    }
}

// TODO: refactor this to use dedicated fake point widget (analogous to the PointDetailsWidget)
void BezierC2Widget::updateSpinboxesForDeBoor(const PointHandle h) {
    const cadm::vec3 pos = m_scene->getPointRegistry().getPosition(h);
    m_spinboxRefreshing = true;
    m_xSpin->setValue(pos.x);
    m_ySpin->setValue(pos.y);
    m_zSpin->setValue(pos.z);
    m_spinboxRefreshing = false;
}

void BezierC2Widget::updateSpinboxesForBernstein(const int index) {
    const auto &positions = m_bezier->getBernsteinPositions();
    if (index < 0 || index >= static_cast<int>(positions.size())) { return; }
    const cadm::vec3 &pos = positions[index];
    m_spinboxRefreshing = true;
    m_xSpin->setValue(pos.x);
    m_ySpin->setValue(pos.y);
    m_zSpin->setValue(pos.z);
    m_spinboxRefreshing = false;
}

void BezierC2Widget::setSpinboxesEnabled(const bool enabled) const {
    m_xSpin->setEnabled(enabled);
    m_ySpin->setEnabled(enabled);
    m_zSpin->setEnabled(enabled);
}
