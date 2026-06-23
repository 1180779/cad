#ifndef ENTITYPROPERTIESWIDGET_H
#define ENTITYPROPERTIESWIDGET_H

#include <QVBoxLayout>
#include <QWidget>
#include "../components/Entity.hpp"
#include "../Scene.hpp"

class CommandStack;

class EntityPropertiesWidget final : public QWidget {
    Q_OBJECT

public:
    explicit EntityPropertiesWidget(QWidget *parent = nullptr);

    void setScene(Scene *scene);

    void setCommandStack(CommandStack *stack) {
        m_commandStack = stack;
    }

    void setEntity(Entity *entity);

signals :
    void propertyChanged();

    void pointSelectionChanged(QList<Entity*> selected);

public
slots :
    /// @brief Mirror the scene's entity selection onto the Bezier point lists
    void syncBezierSelection() const;

    /// @brief Re-sync the component editors after a structural change (points added/
    /// removed/reordered): reconciles the Bezier point lists and refreshes editor values
    void refreshComponents() const;

    /// @brief Re-sync the component editors after a position-only change: refreshes the
    /// editor values but leaves the (position-independent) lists untouched
    void refreshComponentGeometry() const;

private:
    void clearLayout() const;

    Scene *m_scene = nullptr;
    CommandStack *m_commandStack = nullptr;
    Entity *m_entity = nullptr;
    QVBoxLayout *m_layout;
    class PointPropertiesWidget *m_pointWidget = nullptr;
    class BezierC0Widget *m_bezierC0Widget = nullptr;
    class BezierC2Widget *m_bezierC2Widget = nullptr;
    class InterpC2Widget *m_interpC2Widget = nullptr;
};

#endif // ENTITYPROPERTIESWIDGET_H
