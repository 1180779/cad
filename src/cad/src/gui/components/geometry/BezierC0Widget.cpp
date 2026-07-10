//
// Created on 4/18/26.
//

#include "BezierC0Widget.hpp"

#include "Scene.hpp"
#include "../../WidgetBuilders.hxx"
#include "../../../commands/CommandStack.hpp"
#include "../../../commands/Commands.hpp"

using namespace widgets;

BezierC0Widget::BezierC0Widget(BezierC0Component *bezier, Scene *scene, QWidget *parent)
: ComponentWidget(
      bezier,
      parent
  ),
  m_bezier(bezier),
  m_scene(scene) {
    // ReSharper disable once CppDFAMemoryLeak
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);

    addTitle(layout, "Bezier C0 Curve");
    m_showPolygonCheckbox = addCheckbox(layout, "Show control polygon", bezier->getShowPolygon());
    m_pointList = addPointList(layout, "Control points:");
    m_detachButton = addButton(layout, "Remove selected from curve");

    m_pointPropertiesWidget = new PointPropertiesWidget(&scene->getPointRegistry(), this);
    layout->addWidget(m_pointPropertiesWidget);

    populatePointList();

    connect(
        m_showPolygonCheckbox,
        &QCheckBox::toggled,
        this,
        makeBoolToggle(m_bezier, &BezierC0Component::setShowPolygon, m_showPolygonCheckbox)
    );
    connect(m_detachButton, &QPushButton::clicked, this, &BezierC0Widget::onDetachClicked);
    connect(m_pointList, &QListWidget::itemSelectionChanged, this, &BezierC0Widget::onListSelectionChanged);
    connect(m_pointPropertiesWidget, &PointPropertiesWidget::propertyChanged, this, &ComponentWidget::propertyChanged);
}

void BezierC0Widget::onDetachClicked() {
    const int row = m_pointList->currentRow();
    if (row < 0) {
        return;
    }
    const auto &cps = m_bezier->getControlPoints();
    if (row >= static_cast<int>(cps.size())) {
        return;
    }
    if (m_scene && m_commandStack) {
        m_commandStack->push(std::make_unique<RemoveControlPointCommand>(*m_scene, m_entityId, cps[row]));
    }
    else {
        m_bezier->removeControlPointAt(row);
    }
    populatePointList();
    emit propertyChanged();
}

void BezierC0Widget::onListSelectionChanged() {
    const auto selectedItems = m_pointList->selectedItems();
    const PointHandle firstSelectedPoint = selectedItems.isEmpty()
                                               ? InvalidPointHandle
                                               : selectedItems.first()->data(Qt::UserRole).value<PointHandle>();
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
    m_pointPropertiesWidget->setPoint(
        selected.count() == 1
            ? firstSelectedPoint
            : InvalidPointHandle
    );
    emit pointSelectionChanged(selected);
}

void BezierC0Widget::populatePointList() {
    m_pointList->clear();
    m_itemMap.clear();
    const auto &cps = m_bezier->getControlPoints();
    for (int i = 0; i < static_cast<int>(cps.size()); ++i) {
        const auto h = cps[i];
        QString name = QString("Point #%1").arg(h);
        if (m_scene) {
            if (const auto entityOpt = m_scene->getEntityByPointHandle(h)) {
                name = QString::fromStdString(entityOpt.value()->getName());
            }
        }
        const auto item = new QListWidgetItem(QString("[%1] %2").arg(i).arg(name));
        item->setData(Qt::UserRole, QVariant::fromValue(h));
        m_pointList->addItem(item);
        m_itemMap[h] = item;
    }
}

void BezierC0Widget::syncSelection() {
    if (!m_scene) {
        return;
    }
    PointHandle firstSelectedPoint = InvalidPointHandle;
    int selectedCount = 0;
    for (const auto &[h, item] : m_itemMap) {
        if (const auto opt = m_scene->getEntityByPointHandle(h)) {
            const auto isItemSelected = opt.value()->isSelected();
            if (isItemSelected) {
                selectedCount++;
                if (firstSelectedPoint == InvalidPointHandle) {
                    firstSelectedPoint = h;
                }
            }
            item->setSelected(isItemSelected);
        }
    }
    m_pointPropertiesWidget->setPoint(
        selectedCount == 1
            ? firstSelectedPoint
            : InvalidPointHandle
    );
}
