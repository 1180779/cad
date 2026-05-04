//
// Created on 4/18/26.
//

#ifndef CAD_BEZIERC0WIDGET_HPP
#define CAD_BEZIERC0WIDGET_HPP

#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

#include "ComponentWidget.hpp"
#include "../components/BezierC0Component.hpp"
#include "../Scene.hpp"

class BezierC0Widget : public ComponentWidget
{
public:
    explicit BezierC0Widget(BezierC0Component *bezier, Scene *scene, QWidget *parent = nullptr);

    void refreshList() const;

private:
    BezierC0Component *m_bezier;
    Scene *m_scene;
    QCheckBox *m_showPolygonCheckbox{};
    QListWidget *m_pointList{};
    QPushButton *m_removeButton{};
};

#endif //CAD_BEZIERC0WIDGET_HPP
