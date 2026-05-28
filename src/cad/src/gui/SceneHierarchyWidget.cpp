#include "SceneHierarchyWidget.hpp"
#include <QMenu>
#include <QVBoxLayout>
#include <QListWidget>
#include <unordered_set>
#include "../components/BezierC0Component.hpp"
#include "../components/bezierC2Component.hpp"
#include "../components/CursorComponent.hpp"
#include "../components/CameraComponent.hpp"
#include "../components/PointComponent.hpp"

SceneHierarchyWidget::SceneHierarchyWidget(QWidget *parent) : QWidget(parent) {
    const auto layout = new QVBoxLayout(this);
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &SceneHierarchyWidget::onItemSelectionChanged);
    connect(m_listWidget, &QListWidget::itemChanged, this, &SceneHierarchyWidget::onItemChanged);
    connect(
        m_listWidget,
        &QListWidget::customContextMenuRequested,
        this,
        &SceneHierarchyWidget::onContextMenuRequested
    );
}

void SceneHierarchyWidget::setScene(Scene *scene) {
    if (m_scene == scene) { return; }
    m_scene = scene;
    populateList();
}

void SceneHierarchyWidget::addEntityToList(const std::unique_ptr<Entity> &e) const {
    const auto item = new QListWidgetItem(QString::fromStdString(e->getName()));
    item->setData(Qt::UserRole, QVariant::fromValue(e.get()));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_listWidget->addItem(item);
}

void SceneHierarchyWidget::refresh() {
    if (!m_scene) { return; }

    m_refreshing = true;

    std::unordered_set<Entity*> inScene;
    for (const auto &e : m_scene->getEntities()) { inScene.insert(e.get()); }

    // update current entities (delete; update name)
    for (int i = m_listWidget->count() - 1; i >= 0; --i) {
        auto *item = m_listWidget->item(i);
        if (auto *e = item->data(Qt::UserRole).value<Entity*>();
            !inScene.contains(e)) { delete m_listWidget->takeItem(i); }
        else {
            item->setText(QString::fromStdString(e->getName()));
            inScene.erase(e);
        }
    }

    // add missing entities
    for (const auto &e : m_scene->getEntities()) {
        if (!inScene.contains(e.get())) { continue; }
        addEntityToList(e);
    }

    m_refreshing = false;
}

void SceneHierarchyWidget::onItemSelectionChanged() {
    if (m_refreshing) { return; }
    QList<Entity*> selected;
    for (const auto item : m_listWidget->selectedItems()) {
        selected.append(item->data(Qt::UserRole).value<Entity*>());
    }
    emit selectionChanged(selected);
}

void SceneHierarchyWidget::onItemChanged(const QListWidgetItem *item) const {
    // Update entity name
    if (m_refreshing) { return; }
    if (auto *e = item->data(Qt::UserRole).value<Entity*>()) { e->setName(item->text().toStdString()); }
}

void SceneHierarchyWidget::populateList() {
    m_refreshing = true;
    m_listWidget->clear();
    if (!m_scene) {
        m_refreshing = false;
        return;
    }

    for (const auto &e : m_scene->getEntities()) { addEntityToList(e); }
    m_refreshing = false;
}

void SceneHierarchyWidget::setCameraController(CameraController *cameraController) {
    m_cameraController = cameraController;
}

void SceneHierarchyWidget::syncSelectionFromScene() {
    if (!m_scene) { return; }

    m_refreshing = true;
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto *item = m_listWidget->item(i);
        const auto *entity = item->data(Qt::UserRole).value<Entity*>();
        item->setSelected(entity && entity->isSelected());
    }
    m_refreshing = false;
}

void SceneHierarchyWidget::onContextMenuRequested(const QPoint &pos) {
    QMenu menu(this);

    const auto *item = m_listWidget->itemAt(pos);
    if (!item) // create new entities
    {
        const auto *createTorusAction = menu.addAction("New Torus");
        connect(createTorusAction, &QAction::triggered, this, &SceneHierarchyWidget::createTorusRequested);
        const auto *createCursorAction = menu.addAction("New Cursor");
        connect(createCursorAction, &QAction::triggered, this, &SceneHierarchyWidget::createCursorRequested);
        const auto *createPointAction = menu.addAction("New Point");
        connect(createPointAction, &QAction::triggered, this, &SceneHierarchyWidget::createPointRequested);
        menu.addSeparator();
        const auto *createBezierC0Action = menu.addAction("New Bezier C0");
        connect(createBezierC0Action, &QAction::triggered, this, &SceneHierarchyWidget::createBezierC0Requested);
        const auto *createBezierC2Action = menu.addAction("New Bezier C2");
        connect(createBezierC2Action, &QAction::triggered, this, &SceneHierarchyWidget::createBezierC2Requested);
        menu.exec(m_listWidget->mapToGlobal(pos));
        return;
    }

    auto *e = item->data(Qt::UserRole).value<Entity*>();
    if (!e) { return; }

    const bool isCursor = e->hasComponent<CursorComponent>();
    const bool isCamera = e->hasComponent<CameraComponent>();
    const bool isPoint = e->hasComponent<PointComponent>();
    const bool isViableNewPointsTarget = e->hasComponent<INewPointsTargetBase>();
    const bool isActiveCursor = m_scene && m_scene->getActiveCursor() == e;
    const bool isActiveCamera = m_cameraController && m_cameraController->isActiveCamera(e->getId());

    if (isCursor) {
        auto *action = menu.addAction("Set as active cursor");
        action->setEnabled(!isActiveCursor);
        connect(action, &QAction::triggered, this, [this, e] { emit setAsCursorRequested(e); });
    }

    if (isCamera) {
        auto *action = menu.addAction("Set as active camera");
        action->setEnabled(!isActiveCamera);
        connect(action, &QAction::triggered, this, [this, e] { emit setAsCameraRequested(e->getId()); });
    }

    if (isViableNewPointsTarget) {
        const bool isNewPointsTarget = m_scene && m_scene->getNewPointsTargetEntity() == e;
        auto *activeAction = menu.addAction("Set as new points target (auto-add points)");
        activeAction->setEnabled(!isNewPointsTarget);
        connect(activeAction, &QAction::triggered, this, [this, e] { emit setAsNewPointsTargetEntityRequested(e); });
        const auto *addAction = menu.addAction("Add selected points to curve");
        connect(
            addAction,
            &QAction::triggered,
            this,
            [this, e] { emit addSelectedPointsToNewPointsTargetEntityRequested(e); }
        );
    }

    if (!isCamera) {
        const auto *focusAction = menu.addAction("Focus camera");
        connect(focusAction, &QAction::triggered, this, [this, e] { emit focusCameraRequested(e); });
    }

    if (!isCursor && !isCamera) {
        const auto *deleteAction = menu.addAction("Delete");
        connect(deleteAction, &QAction::triggered, this, [this, e] { emit deleteEntityRequested(e); });
    }

    menu.exec(m_listWidget->mapToGlobal(pos));
}
