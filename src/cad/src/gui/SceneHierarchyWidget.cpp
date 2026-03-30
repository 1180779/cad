#include "SceneHierarchyWidget.hpp"
#include <QVBoxLayout>
#include <QListWidget>
#include <unordered_set>

SceneHierarchyWidget::SceneHierarchyWidget(QWidget *parent)
    : QWidget(parent)
{
    const auto layout = new QVBoxLayout(this);
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &SceneHierarchyWidget::onItemSelectionChanged);
    connect(m_listWidget, &QListWidget::itemChanged, this, &SceneHierarchyWidget::onItemChanged);
}

void SceneHierarchyWidget::setScene(Scene *scene)
{
    if (m_scene == scene) return;
    m_scene = scene;
    populateList();
}

void SceneHierarchyWidget::addEntityToList(const std::unique_ptr<entity> &e) const
{
    const auto item = new QListWidgetItem(QString::fromStdString(e->getName()));
    item->setData(Qt::UserRole, QVariant::fromValue(e.get()));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_listWidget->addItem(item);
}

void SceneHierarchyWidget::refresh()
{
    if (!m_scene) return;

    m_refreshing = true;

    std::unordered_set<entity*> inScene;
    for (const auto &e : m_scene->getEntities())
        inScene.insert(e.get());

    // Update current entities (delete; update name)
    for (int i = m_listWidget->count() - 1; i >= 0; --i)
    {
        auto *item = m_listWidget->item(i);
        if (auto *e = item->data(Qt::UserRole).value<entity*>(); !inScene.contains(e))
        {
            delete m_listWidget->takeItem(i);
        }
        else
        {
            item->setText(QString::fromStdString(e->getName()));
            inScene.erase(e);
        }
    }

    // Add missing entities
    for (const auto &e : m_scene->getEntities())
    {
        if (!inScene.contains(e.get())) continue;
        addEntityToList(e);
    }

    m_refreshing = false;
}

void SceneHierarchyWidget::onItemSelectionChanged()
{
    QList<entity*> selected;
    for (const auto item : m_listWidget->selectedItems())
        selected.append(item->data(Qt::UserRole).value<entity*>());
    emit selectionChanged(selected);
}

void SceneHierarchyWidget::onItemChanged(const QListWidgetItem *item) const
{
    // Update entity name
    if (m_refreshing) return;
    if (auto *e = item->data(Qt::UserRole).value<entity*>())
        e->setName(item->text().toStdString());
}

void SceneHierarchyWidget::populateList()
{
    m_refreshing = true;
    m_listWidget->clear();
    if (!m_scene)
    {
        m_refreshing = false;
        return;
    }

    for (const auto &e : m_scene->getEntities())
    {
        addEntityToList(e);
    }
    m_refreshing = false;
}
