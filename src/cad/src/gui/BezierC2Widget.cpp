//
// Created on 5/5/26.
//

#include "BezierC2Widget.hpp"

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

    /// @brief Label for a Bernstein row in the shared-endpoint layout
    QString bernsteinLabel(const int i, const int segments) {
        const int seg = i / 3;
        if (const int local = i % 3;
            local != 0) {
            return QString("b%1 [seg %2]").arg(local).arg(seg);
        }
        if (i == 0) {
            return "b0 [seg 0]";
        }
        if (seg == segments) {
            return QString("b3 [seg %1]").arg(seg - 1);
        }
        return QString("b3 [seg %1] / b0 [seg %2]").arg(seg - 1).arg(seg);
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

    m_showBernsteinCpsCheckbox = new QCheckBox("Show Bernstein control points");
    m_showBernsteinCpsCheckbox->setChecked(bezier->getShowBernsteinCps());
    layout->addWidget(m_showBernsteinCpsCheckbox);

    // de Boor list
    layout->addWidget(new QLabel("De Boor points:"));
    m_deBoorList = new QListWidget;
    m_deBoorList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_deBoorList->setMaximumHeight(120);
    layout->addWidget(m_deBoorList);

    m_removeButton = new QPushButton("Remove selected from curve");
    layout->addWidget(m_removeButton);

    // ReSharper disable once CppDFAMemoryLeak
    auto *sep1 = new QFrame;
    sep1->setFrameShape(QFrame::HLine);
    layout->addWidget(sep1);

    // Bernstein list
    layout->addWidget(new QLabel("Bernstein points:"));
    m_bernsteinList = new QListWidget;
    m_bernsteinList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_bernsteinList->setMaximumHeight(120);
    layout->addWidget(m_bernsteinList);

    // ReSharper disable once CppDFAMemoryLeak
    auto *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    layout->addWidget(sep2);

    // shared editor for the selected de Boor / Bernstein point
    m_pointProps = new VirtualPointPropertiesWidget;
    layout->addWidget(m_pointProps);
    m_pointProps->setActive(false);

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
        m_showBernsteinCpsCheckbox,
        &QCheckBox::toggled,
        this,
        [this](const bool checked) {
            auto *b = m_bezier;
            pushEdit(
                [b, checked] {
                    b->setShowBernsteinCps(checked);
                },
                [b, checked] {
                    b->setShowBernsteinCps(!checked);
                },
                m_showBernsteinPolygonCheckbox,
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
                m_selectedKind == SelectedPointKind::deBoor && m_selectedDeBoorHandle == removing) {
                m_selectedKind = SelectedPointKind::none;
                m_selectedDeBoorHandle = InvalidPointHandle;
                m_pointProps->setActive(false);
            }
            if (const auto &cps = m_bezier->getDeBoorPoints();
                m_scene && m_commandStack && row < static_cast<int>(cps.size())) {
                m_commandStack->push(std::make_unique<RemoveControlPointCommand>(*m_scene, m_entityId, cps[row]));
            }
            else {
                m_bezier->removeControlPointAt(row);
            }
            reconcileDeBoorList();
            reconcileBernsteinList();
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
                    m_pointProps->setActive(false);
                }
                return;
            }

            // clear bernstein selection
            const QSignalBlocker b(m_bernsteinList);
            m_bernsteinList->clearSelection();

            m_selectedKind = SelectedPointKind::deBoor;
            m_selectedDeBoorHandle = cur->data(Qt::UserRole).value<PointHandle>();
            m_selectedBernsteinIndex = -1;
            m_pointProps->setActive(true);
            updateSpinboxesForDeBoor(m_selectedDeBoorHandle);
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
                    m_pointProps->setActive(false);
                }
                return;
            }

            // clear de Boor selection
            const QSignalBlocker b(m_deBoorList);
            m_deBoorList->clearSelection();

            m_selectedKind = SelectedPointKind::bernstein;
            m_selectedDeBoorHandle = InvalidPointHandle;
            m_selectedBernsteinIndex = cur->data(Qt::UserRole).toInt();
            m_pointProps->setActive(true);
            updateSpinboxesForBernstein(m_selectedBernsteinIndex);
        }
    );

    connect(
        m_pointProps,
        &VirtualPointPropertiesWidget::coordinateEdited,
        this,
        &BezierC2Widget::onCoordinateEdited
    );
}

void BezierC2Widget::onCoordinateEdited(const cadm::Vec3 newPos) {
    auto &reg = m_scene->getPointRegistry();
    if (m_selectedKind == SelectedPointKind::deBoor &&
        m_selectedDeBoorHandle != InvalidPointHandle) {
        // moving a de Boor point is a plain point move (coalesces by handle)
        if (m_commandStack) {
            const auto before = reg.getPosition(m_selectedDeBoorHandle);
            m_commandStack->push(
                std::make_unique<MovePointCommand>(*m_scene, m_selectedDeBoorHandle, before, newPos),
                true
            );
        }
        else {
            reg.setPosition(m_selectedDeBoorHandle, newPos);
        }
        emit propertyChanged();
    }
    else if (m_selectedKind == SelectedPointKind::bernstein &&
        m_selectedBernsteinIndex >= 0) {
        // a Bernstein edit back-computes several de Boor points; snapshot them
        // so undo can restore them; apply re-runs the back-computation
        auto *lBezier = m_bezier;
        auto *lScene = m_scene;
        const int idx = m_selectedBernsteinIndex;
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
    const int segments = m_bezier->segmentCount();
    for (int i = 0; i < count; ++i) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto item = new QListWidgetItem(bernsteinLabel(i, segments));
        item->setData(Qt::UserRole, i);
        m_bernsteinList->addItem(item);
    }

    // re-select previously selected index if still valid
    if (m_selectedKind == SelectedPointKind::bernstein &&
        m_selectedBernsteinIndex >= 0 && m_selectedBernsteinIndex < count) {
        const QSignalBlocker b(m_bernsteinList);
        m_bernsteinList->setCurrentRow(m_selectedBernsteinIndex);
    }
}

void BezierC2Widget::reconcileBernsteinList() const {
    const int target = static_cast<int>(m_bezier->getBernsteinPositions().size());

    const QSignalBlocker blocker(m_bernsteinList);

    // labels depend only on the row index, so the only structural change is the count:
    // drop surplus trailing rows, append any missing ones, leave existing rows as-is
    // ReSharper disable once CppDFALoopConditionNotUpdated
    while (m_bernsteinList->count() > target) {
        delete m_bernsteinList->takeItem(m_bernsteinList->count() - 1);
    }
    const int segments = m_bezier->segmentCount();
    for (int i = m_bernsteinList->count(); i < target; ++i) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto item = new QListWidgetItem(bernsteinLabel(i, segments));
        item->setData(Qt::UserRole, i);
        m_bernsteinList->addItem(item);
    }

    // keep the tracked selection shown (no-op when the row is already current)
    if (m_selectedKind == SelectedPointKind::bernstein &&
        m_selectedBernsteinIndex >= 0 && m_selectedBernsteinIndex < target &&
        m_bernsteinList->currentRow() != m_selectedBernsteinIndex) {
        m_bernsteinList->setCurrentRow(m_selectedBernsteinIndex);
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
    reconcileBernsteinList();
    refreshSelectedSpinboxes();
}

void BezierC2Widget::refreshGeometry() const {
    refreshSelectedSpinboxes();
}

void BezierC2Widget::refreshSelectedSpinboxes() const {
    if (m_selectedKind == SelectedPointKind::deBoor &&
        m_selectedDeBoorHandle != InvalidPointHandle) {
        updateSpinboxesForDeBoor(m_selectedDeBoorHandle);
    }
    else if (m_selectedKind == SelectedPointKind::bernstein &&
        m_selectedBernsteinIndex >= 0 &&
        m_selectedBernsteinIndex < static_cast<int>(m_bezier->getBernsteinPositions().size())) {
        updateSpinboxesForBernstein(m_selectedBernsteinIndex);
    }
}

void BezierC2Widget::updateSpinboxesForDeBoor(const PointHandle h) const {
    m_pointProps->setPosition(m_scene->getPointRegistry().getPosition(h));
}

void BezierC2Widget::updateSpinboxesForBernstein(const int index) const {
    m_bezier->ensureBernsteinUpToDate();
    const auto &positions = m_bezier->getBernsteinPositions();
    if (index < 0 || index >= static_cast<int>(positions.size())) {
        return;
    }
    m_pointProps->setPosition(positions[index]);
}
