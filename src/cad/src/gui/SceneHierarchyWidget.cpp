#include "SceneHierarchyWidget.h"
#include <QVBoxLayout>
#include <QListWidget>

SceneHierarchyWidget::SceneHierarchyWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
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

void SceneHierarchyWidget::populateList()
{
    m_listWidget->clear();
    if (!m_scene) return;

    for (const auto &entity : m_scene->getEntities())
    {
        auto item = new QListWidgetItem(QString::fromStdString(entity->getName()));
        item->setData(Qt::UserRole, QVariant::fromValue(entity.get()));
        m_listWidget->addItem(item);
    }
}

void SceneHierarchyWidget::onCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
    {
        emit entitySelected(nullptr);
        return;
    }

    auto selectedEntity = current->data(Qt::UserRole).value<entity*>();
    emit entitySelected(selectedEntity);
}