//
// Created by Radosław Głasek on 23.06.2026
//

#include <unordered_set>

#include "InterpC2Widget.hxx"
#include "../../../commands/CommandStack.hpp"
#include "../../../commands/Commands.hpp"
#include "../../WidgetBuilders.hxx"

using namespace widgets;

namespace {
    /// @brief Label shown for the row at position i, resolving the entity name
    QString pointLabel(Scene *scene, int i, PointHandle h);
}

InterpC2Widget::InterpC2Widget(InterpC2Component *curve, Scene *scene, QWidget *parent) : ComponentWidget(
        curve,
        parent
    ),
    m_curve(curve),
    m_scene(scene) {
    // ReSharper disable once CppDFAMemoryLeak
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    addTitle(layout, "Interpolating C2 Curve");
    m_showPolylineCheckbox = addCheckbox(layout, "Show interpolation polyline", curve->getShowControlPolyline());
    m_showBernsteinPolygonCheckbox = addCheckbox(layout, "Show Bernstein polygon", curve->getShowBernsteinPolygon());
    m_showBernsteinCpsCheckbox = addCheckbox(layout, "Show Bernstein control points", curve->getShowBernsteinCps());
    m_pointList = addPointList(layout, "Interpolated points:");
    m_removeButton = addButton(layout, "Remove selected from curve");
    addSeparator(layout);
    m_pointProps = addPointProps(layout);

    refreshList();

    connect(
        m_showPolylineCheckbox,
        &QCheckBox::toggled,
        this,
        makeBoolToggle(m_curve, &InterpC2Component::setShowControlPolyline, m_showPolylineCheckbox)
    );
    connect(
        m_showBernsteinPolygonCheckbox,
        &QCheckBox::toggled,
        this,
        makeBoolToggle(m_curve, &InterpC2Component::setShowBernsteinPolygon, m_showBernsteinPolygonCheckbox)
    );
    connect(
        m_showBernsteinCpsCheckbox,
        &QCheckBox::toggled,
        this,
        makeBoolToggle(m_curve, &InterpC2Component::setShowBernsteinCps, m_showBernsteinCpsCheckbox)
    );
    connect(m_removeButton, &QPushButton::clicked, this, &InterpC2Widget::onRemoveClicked);
    connect(m_pointList, &QListWidget::itemSelectionChanged, this, &InterpC2Widget::onListSelectionChanged);
    connect(m_pointProps, &VirtualPointPropertiesWidget::coordinateEdited, this, &InterpC2Widget::onCoordinateEdited);
}

void InterpC2Widget::onRemoveClicked() {
    const int row = m_pointList->currentRow();
    if (row < 0) {
        return;
    }
    const auto &cps = m_curve->getControlPoints();
    if (row >= static_cast<int>(cps.size())) {
        return;
    }
    if (m_selectedHandle == cps[row]) {
        m_selectedHandle = InvalidPointHandle;
        m_pointProps->setActive(false);
    }
    if (m_scene && m_commandStack) {
        m_commandStack->push(std::make_unique<RemoveControlPointCommand>(*m_scene, m_entityId, cps[row]));
    }
    else {
        m_curve->removeControlPointAt(row);
    }
    reconcileList();
    emit propertyChanged();
}

void InterpC2Widget::onListSelectionChanged() {
    if (!m_scene) {
        return;
    }

    QList<Entity*> selected;
    for (const auto *item : m_pointList->selectedItems()) {
        const auto h = item->data(Qt::UserRole).value<PointHandle>();
        if (const auto opt = m_scene->getEntityByPointHandle(h)) {
            selected.append(opt.value());
        }
    }
    emit pointSelectionChanged(selected);

    const auto *cur = m_pointList->currentItem();
    if (!cur || !cur->isSelected()) {
        m_selectedHandle = InvalidPointHandle;
        m_pointProps->setActive(false);
        return;
    }
    m_selectedHandle = cur->data(Qt::UserRole).value<PointHandle>();
    m_pointProps->setActive(true);
    updateSpinboxes(m_selectedHandle);
}

void InterpC2Widget::onCoordinateEdited(const cadm::Vec3 newPos) {
    if (m_selectedHandle == InvalidPointHandle) {
        return;
    }
    auto &reg = m_scene->getPointRegistry();
    if (m_commandStack) {
        const auto before = reg.getPosition(m_selectedHandle);
        m_commandStack->push(
            std::make_unique<MovePointCommand>(*m_scene, m_selectedHandle, before, newPos),
            true
        );
    }
    else {
        reg.setPosition(m_selectedHandle, newPos);
    }
    emit propertyChanged();
}

void InterpC2Widget::refreshList() {
    m_pointList->clear();
    m_itemMap.clear();
    const auto &pts = m_curve->getControlPoints();
    for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
        const PointHandle h = pts[i];
        const auto item = new QListWidgetItem(pointLabel(m_scene, i, h));
        item->setData(Qt::UserRole, QVariant::fromValue(h));
        m_pointList->addItem(item);
        m_itemMap[h] = item;
    }
}

void InterpC2Widget::reconcileList() {
    const auto &handles = m_curve->getControlPoints();

    const QSignalBlocker blocker(m_pointList);

    const std::unordered_set live(handles.begin(), handles.end());

    for (auto it = m_itemMap.begin(); it != m_itemMap.end();) {
        if (!live.contains(it->first)) {
            delete m_pointList->takeItem(m_pointList->row(it->second));
            it = m_itemMap.erase(it);
        }
        else {
            ++it;
        }
    }

    for (std::size_t i = 0; i < handles.size(); ++i) {
        const auto h = handles[i];
        QListWidgetItem *item;
        if (const auto found = m_itemMap.find(h);
            found == m_itemMap.end()) {
            item = new QListWidgetItem;
            item->setData(Qt::UserRole, QVariant::fromValue(h));
            m_pointList->insertItem(static_cast<int>(i), item);
            m_itemMap[h] = item;
        }
        else {
            item = found->second;
            if (const int row = m_pointList->row(item);
                row != static_cast<int>(i)) {
                const bool wasSelected = item->isSelected();
                m_pointList->takeItem(row);
                m_pointList->insertItem(static_cast<int>(i), item);
                item->setSelected(wasSelected);
            }
        }

        if (const QString label = pointLabel(m_scene, static_cast<int>(i), h);
            item->text() != label) {
            item->setText(label);
        }
    }
}

void InterpC2Widget::syncSelection() {
    if (!m_scene) {
        return;
    }
    for (const auto &[h, item] : m_itemMap) {
        if (const auto opt = m_scene->getEntityByPointHandle(h)) {
            item->setSelected(opt.value()->isSelected());
        }
    }
}

void InterpC2Widget::refresh() {
    reconcileList();
    refreshGeometry();
}

void InterpC2Widget::refreshGeometry() const {
    if (m_selectedHandle != InvalidPointHandle) {
        updateSpinboxes(m_selectedHandle);
    }
}

void InterpC2Widget::updateSpinboxes(const PointHandle h) const {
    m_pointProps->setPosition(m_scene->getPointRegistry().getPosition(h));
}

namespace {
    QString pointLabel(Scene *scene, const int i, const PointHandle h) {
        QString name = QString("Point #%1").arg(h);
        if (scene) {
            if (const auto entityOpt = scene->getEntityByPointHandle(h)) {
                name = QString::fromStdString(entityOpt.value()->getName());
            }
        }
        return QString("[%1] %2").arg(i).arg(name);
    }
}
