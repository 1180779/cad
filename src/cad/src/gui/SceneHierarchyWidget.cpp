#include "SceneHierarchyWidget.hpp"
#include <QVBoxLayout>
#include <QListWidget>

SceneHierarchyWidget::SceneHierarchyWidget(QWidget *parent)
    : QWidget(parent)
{
    const auto layout = new QVBoxLayout(this);
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &SceneHierarchyWidget::onItemSelectionChanged);
}

void SceneHierarchyWidget::setScene(Scene *scene)
{
    if (m_scene == scene) return;
    m_scene = scene;
    populateList();
}

void SceneHierarchyWidget::onItemSelectionChanged()
{
    QList<entity*> selected;
    for (const auto item : m_listWidget->selectedItems())
        selected.append(item->data(Qt::UserRole).value<entity*>());
    emit selectionChanged(selected);
}

void SceneHierarchyWidget::populateList() const
{
    m_listWidget->clear();
    if (!m_scene) return;

    for (const auto &entity : m_scene->getEntities())
    {
        const auto item = new QListWidgetItem(QString::fromStdString(entity->getName()));
        item->setData(Qt::UserRole, QVariant::fromValue(entity.get()));
        m_listWidget->addItem(item);
    }
}
