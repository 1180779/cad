//
// Created on 4/18/26.
//

#include "BezierC0Widget.hpp"

#include <QLabel>
#include <QVBoxLayout>

#include "Scene.hpp"
#include "../commands/CommandStack.hpp"
#include "../commands/Commands.hpp"

BezierC0Widget::BezierC0Widget(BezierC0Component *bezier, Scene *scene, QWidget *parent) : ComponentWidget(
        bezier,
        parent
    ),
    m_bezier(bezier),
    m_scene(scene) {
    // ReSharper disable once CppDFAMemoryLeak

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    // ReSharper disable once CppDFAMemoryLeak

    const auto titleLabel = new QLabel("Bezier C0 Curve");
    QFont f = titleLabel->font();
    f.setBold(true);
    titleLabel->setFont(f);
    layout->addWidget(titleLabel);

    m_showPolygonCheckbox = new QCheckBox("Show control polygon");
    m_showPolygonCheckbox->setChecked(bezier->getShowPolygon());
    layout->addWidget(m_showPolygonCheckbox);

    layout->addWidget(new QLabel("Control points:"));
    m_pointList = new QListWidget;
    m_pointList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pointList->setMaximumHeight(120);
    layout->addWidget(m_pointList);

    m_detachButton = new QPushButton("Remove selected from curve");
    layout->addWidget(m_detachButton);

    m_pointPropertiesWidget = new PointPropertiesWidget(&scene->getPointRegistry(), this);
    layout->addWidget(m_pointPropertiesWidget);

    populatePointList();

    connect(
        m_showPolygonCheckbox,
        &QCheckBox::toggled,
        this,
        [this](const bool checked) {
            auto *b = m_bezier;
            pushEdit(
                [b, checked] {
                    b->setShowPolygon(checked);
                },
                [b, checked] {
                    b->setShowPolygon(!checked);
                },
                m_showPolygonCheckbox,
                false
            );
        }
    );

    connect(
        m_detachButton,
        &QPushButton::clicked,
        this,
        [this] {
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
    );

    connect(
        m_pointList,
        &QListWidget::itemSelectionChanged,
        this,
        [this] {
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
    );

    connect(m_pointPropertiesWidget, &PointPropertiesWidget::propertyChanged, this, &ComponentWidget::propertyChanged);
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

void BezierC0Widget::syncSelectionFromScene() {
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
