//
// Created on 4/18/26.
//

#include "BezierC0Widget.hpp"

#include <QLabel>
#include <QVBoxLayout>

BezierC0Widget::BezierC0Widget(BezierC0Component *bezier, Scene *scene, QWidget *parent)
    : ComponentWidget(bezier, parent), m_bezier(bezier), m_scene(scene)
{
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

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
    m_pointList->setMaximumHeight(120);
    layout->addWidget(m_pointList);

    m_removeButton = new QPushButton("Remove selected from curve");
    layout->addWidget(m_removeButton);

    refreshList();

    connect(
        m_showPolygonCheckbox,
        &QCheckBox::toggled,
        this,
        [this](const bool checked)
        {
            m_bezier->setShowPolygon(checked);
            emit propertyChanged();
        });

    connect(
        m_removeButton,
        &QPushButton::clicked,
        this,
        [this]
        {
            const int row = m_pointList->currentRow();
            if (row < 0) return;
            m_bezier->removeControlPointAt(row);
            refreshList();
            emit propertyChanged();
        });
}

void BezierC0Widget::refreshList() const
{
    m_pointList->clear();
    const auto &cps = m_bezier->getControlPoints();
    for (int i = 0; i < static_cast<int>(cps.size()); ++i)
    {
        const PointHandle h = cps[i];
        // Look up entity name if available
        QString name = QString("Point #%1").arg(h);
        if (m_scene)
        {
            if (const auto entityOpt = m_scene->getEntityByPointHandle(h))
            {
                name = QString::fromStdString(entityOpt.value()->getName());
            }
        }
        m_pointList->addItem(QString("[%1] %2").arg(i).arg(name));
    }
}
