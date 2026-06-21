//
// Created on 5/5/26.
//

#include "BezierC2Widget.hpp"

#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <unordered_set>

#include "../commands/CommandStack.hpp"
#include "../commands/Commands.hpp"

namespace {
    /// @brief Label shown for a de Boor row at position i, resolving the entity name
    QString deBoorLabel(Scene *scene, const int i, const PointHandle h) {
        QString name = QString("Point #%1").arg(h);
        if (scene) {
            if (const auto entityOpt = scene->getEntityByPointHandle(h)) {
                name = QString::fromStdString(entityOpt.value()->getName());
            }
        }
        return QString("[%1] %2").arg(i).arg(name);
    }
}

BezierC2Widget::BezierC2Widget(BezierC2Component *bezier, Scene *scene, QWidget *parent) : ComponentWidget(
        bezier,
        parent
    ),
    m_bezier(bezier),
    m_scene(scene) {
    // ReSharper disable once CppDFAMemoryLeak
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // ReSharper disable once CppDFAMemoryLeak
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

    // ReSharper disable once CppDFAMemoryLeak
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
            auto *b = m_bezier;
            pushEdit(
                [b, checked] {
                    b->setShowDeBoorPolygon(checked);
                },
                [b, checked] {
                    b->setShowDeBoorPolygon(!checked);
                },
                m_showDeBoorPolygonCheckbox,
                false
            );
        }
    );

    connect(
        m_showBernsteinPolygonCheckbox,
        &QCheckBox::toggled,
        this,
        [this](const bool checked) {
            auto *b = m_bezier;
            pushEdit(
                [b, checked] {
                    b->setShowBernsteinPolygon(checked);
                },
                [b, checked] {
                    b->setShowBernsteinPolygon(!checked);
                },
                m_showBernsteinPolygonCheckbox,
                false
            );
        }
    );

    connect(
        m_uniformCheckbox,
        &QCheckBox::toggled,
        this,
        [this](const bool checked) {
            auto *b = m_bezier;
            const auto before = b->getParametrizationMode();
            const auto after = checked
                                   ? ParametrizationMode::uniform
                                   : ParametrizationMode::chordLength;
            pushEdit(
                [b, after] {
                    b->setParametrizationMode(after);
                },
                [b, before] {
                    b->setParametrizationMode(before);
                },
                m_uniformCheckbox,
                false
            );
        }
    );

    connect(
        m_removeButton,
        &QPushButton::clicked,
        this,
        [this] {
            const int row = m_deBoorList->currentRow();
            if (row < 0) {
                return;
            }
            if (const PointHandle removing = m_bezier->getDeBoorPoints()[row];
                m_selectedKind == SelectedPointKind::deBoor && m_selectedDeBoor == removing) {
                m_selectedKind = SelectedPointKind::none;
                m_selectedDeBoor = InvalidPointHandle;
                setSpinboxesEnabled(false);
                m_selectedLabel->setEnabled(false);
            }
            const auto &cps = m_bezier->getDeBoorPoints();
            if (m_scene && m_commandStack &&row<static_cast<int>(cps.size())) {
                m_commandStack->push(std::make_unique<RemoveControlPointCommand>(*m_scene, m_entityId, cps[row]));
            }
            else {
                m_bezier->removeControlPointAt(row);
            }
            refreshList();
            emit propertyChanged();
        }
    );

    connect(
        m_deBoorList,
        &QListWidget::itemSelectionChanged,
        this,
        [this] {
            if (!m_scene) {
                return;
            }

            QList<Entity*> selected;
            for (const auto *item : m_deBoorList->selectedItems()) {
                const auto h = item->data(Qt::UserRole).value<PointHandle>();
                if (const auto opt = m_scene->getEntityByPointHandle(h)) {
                    selected.append(opt.value());
                }
            }
            emit pointSelectionChanged(selected);

            // update spinboxes for the single current item
            const auto *cur = m_deBoorList->currentItem();
            if (!cur || !cur->isSelected()) {
                if (m_selectedKind == SelectedPointKind::deBoor) {
                    m_selectedKind = SelectedPointKind::none;
                    setSpinboxesEnabled(false);
                    m_selectedLabel->setEnabled(false);
                }
                return;
            }

            // clear bernstein selection
            const QSignalBlocker b(m_bernsteinList);
            m_bernsteinList->clearSelection();

            m_selectedKind = SelectedPointKind::deBoor;
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
                if (m_selectedKind == SelectedPointKind::bernstein) {
                    m_selectedKind = SelectedPointKind::none;
                    setSpinboxesEnabled(false);
                    m_selectedLabel->setEnabled(false);
                }
                return;
            }

            // clear de Boor selection
            const QSignalBlocker b(m_deBoorList);
            m_deBoorList->clearSelection();

            m_selectedKind = SelectedPointKind::bernstein;
            m_selectedDeBoor = InvalidPointHandle;
            m_selectedBernstein = cur->data(Qt::UserRole).toInt();
            m_selectedLabel->setEnabled(true);
            updateSpinboxesForBernstein(m_selectedBernstein);
            setSpinboxesEnabled(true);
        }
    );

    // spinbox edits
    auto onSpinChanged = [this] {
        if (m_spinboxRefreshing) {
            return;
        }
        const cadm::Vec3 newPos{
            static_cast<float>(m_xSpin->value()),
            static_cast<float>(m_ySpin->value()),
            static_cast<float>(m_zSpin->value())
        };
        auto &reg = m_scene->getPointRegistry();
        if (m_selectedKind == SelectedPointKind::deBoor &&
            m_selectedDeBoor != InvalidPointHandle) {
            // moving a de Boor point is a plain point move (coalesces by handle)
            if (m_commandStack) {
                const auto before = reg.getPosition(m_selectedDeBoor);
                m_commandStack->push(
                    std::make_unique<MovePointCommand>(*m_scene, m_selectedDeBoor, before, newPos),
                    true
                );
            }
            else {
                reg.setPosition(m_selectedDeBoor, newPos);
            }
            emit propertyChanged();
        }
        else if (m_selectedKind == SelectedPointKind::bernstein &&
            m_selectedBernstein >= 0) {
            // a Bernstein edit back-computes several de Boor points; snapshot them
            // so undo can restore the lot. apply re-runs the back-computation
            auto *lBezier = m_bezier;
            auto *lScene = m_scene;
            const int idx = m_selectedBernstein;
            std::vector<std::pair<PointHandle, cadm::Vec3>> before;
            for (const auto h : lBezier->getDeBoorPoints()) {
                before.emplace_back(h, reg.getPosition(h));
            }
            pushEdit(
                [lBezier, lScene, idx, newPos] {
                    lBezier->setBernsteinPosition(idx, newPos);
                    lScene->getPointRegistry().syncToGpu();
                },
                [lScene, before] {
                    for (const auto &[h, p] : before) {
                        lScene->getPointRegistry().setPosition(h, p);
                    }
                },
                m_bernsteinList,
                true
            );
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
        const auto item = new QListWidgetItem(deBoorLabel(m_scene, i, h));
        item->setData(Qt::UserRole, QVariant::fromValue(h));
        m_deBoorList->addItem(item);
        m_deBoorItemMap[h] = item;
    }

    refreshBernsteinList();
}

void BezierC2Widget::reconcileDeBoorList() {
    const auto &handles = m_bezier->getDeBoorPoints();

    // block signals: insert/remove/reorder would otherwise fire itemSelectionChanged
    const QSignalBlocker blocker(m_deBoorList);

    const std::unordered_set live(handles.begin(), handles.end());

    // drop rows whose point is gone from the curve
    for (auto it = m_deBoorItemMap.begin(); it != m_deBoorItemMap.end();) {
        if (!live.contains(it->first)) {
            delete m_deBoorList->takeItem(m_deBoorList->row(it->second));
            it = m_deBoorItemMap.erase(it);
        }
        else {
            ++it;
        }
    }

    // walk the target order, reusing/creating/moving rows into place
    for (std::size_t i = 0; i < handles.size(); ++i) {
        const auto h = handles[i];
        QListWidgetItem *item;
        // insert new row if there's no associated one already
        if (const auto found = m_deBoorItemMap.find(h);
            found == m_deBoorItemMap.end()) {
            item = new QListWidgetItem;
            item->setData(Qt::UserRole, QVariant::fromValue(h));
            m_deBoorList->insertItem(static_cast<int>(i), item);
            m_deBoorItemMap[h] = item;
        }
        else {
            // row exists; reorder it if needed
            item = found->second;
            if (const int row = m_deBoorList->row(item);
                row != i) {
                const bool wasSelected = item->isSelected();
                m_deBoorList->takeItem(row);
                m_deBoorList->insertItem(static_cast<int>(i), item);
                item->setSelected(wasSelected);
            }
        }

        if (const QString label = deBoorLabel(m_scene, static_cast<int>(i), h);
            item->text() != label) {
            item->setText(label);
        }
    }
}

void BezierC2Widget::refreshBernsteinList() const {
    m_bernsteinList->clear();
    const auto &positions = m_bezier->getBernsteinPositions();
    const int count = static_cast<int>(positions.size());
    for (int i = 0; i < count; ++i) {
        const int seg = i / 4;
        const int local = i % 4;
        // ReSharper disable once CppDFAMemoryLeak
        const auto item = new QListWidgetItem(
            QString("b%1 [seg %2]").arg(local).arg(seg)
        );
        item->setData(Qt::UserRole, i);
        m_bernsteinList->addItem(item);
    }

    // re-select previously selected index if still valid
    if (m_selectedKind == SelectedPointKind::bernstein &&
        m_selectedBernstein >= 0 && m_selectedBernstein < count) {
        const QSignalBlocker b(m_bernsteinList);
        m_bernsteinList->setCurrentRow(m_selectedBernstein);
    }
}

void BezierC2Widget::syncSelection() {
    if (!m_scene) {
        return;
    }
    for (const auto &[h, item] : m_deBoorItemMap) {
        if (const auto opt = m_scene->getEntityByPointHandle(h)) {
            item->setSelected(opt.value()->isSelected());
        }
    }
}

void BezierC2Widget::refresh() {
    reconcileDeBoorList();
    refreshBernsteinList();

    if (m_selectedKind == SelectedPointKind::deBoor &&
        m_selectedDeBoor != InvalidPointHandle) {
        updateSpinboxesForDeBoor(m_selectedDeBoor);
    }
    else if (m_selectedKind == SelectedPointKind::bernstein &&
        m_selectedBernstein >= 0 &&
        m_selectedBernstein < static_cast<int>(m_bezier->getBernsteinPositions().size())) {
        updateSpinboxesForBernstein(m_selectedBernstein);
    }
}

// TODO: refactor this to use dedicated fake point widget (analogous to the PointDetailsWidget)
void BezierC2Widget::updateSpinboxesForDeBoor(const PointHandle h) {
    const cadm::Vec3 pos = m_scene->getPointRegistry().getPosition(h);
    m_spinboxRefreshing = true;
    m_xSpin->setValue(pos.x);
    m_ySpin->setValue(pos.y);
    m_zSpin->setValue(pos.z);
    m_spinboxRefreshing = false;
}

void BezierC2Widget::updateSpinboxesForBernstein(const int index) {
    const auto &positions = m_bezier->getBernsteinPositions();
    if (index < 0 || index >= static_cast<int>(positions.size())) {
        return;
    }
    const cadm::Vec3 &pos = positions[index];
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
