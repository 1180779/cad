#include "SceneHierarchyWidget.h"
#include <QVBoxLayout>
#include <QListWidget>

SceneHierarchyWidget::SceneHierarchyWidget(QWidget *parent)
    : QWidget(parent)
{
    const auto layout = new QVBoxLayout(this);
    m_listWidget = new QListWidget(this);
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::currentItemChanged, this, &SceneHierarchyWidget::onCurrentItemChanged);
}

void SceneHierarchyWidget::setScene(Scene *scene)
{
    if (m_scene == scene) return;
    m_scene = scene;
    populateList();
}

void SceneHierarchyWidget::onCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
    {
        emit entitySelected(nullptr);
        return;
    }

    const auto selectedEntity = current->data(Qt::UserRole).value<entity*>();
    emit entitySelected(selectedEntity);
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
