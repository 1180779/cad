#ifndef ENTITYPROPERTIESWIDGET_H
#define ENTITYPROPERTIESWIDGET_H

#include <QVBoxLayout>
#include <QWidget>
#include "../../../components/Entity.hpp"
#include "../../../Scene.hpp"

class CommandStack;
class EntityPropertiesWidget;

namespace aliases {
    using EntPropsW = EntityPropertiesWidget;
}

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
    void syncBezierSelection();

    /// @brief Re-sync the component editors after a structural change (points added/
    /// removed/reordered): reconciles the Bezier point lists and refreshes editor values
    void refreshComponents();

    /// @brief Re-sync the component editors after a position-only change: refreshes the
    /// editor values but leaves the (position-independent) lists untouched
    void refreshComponentGeometry();

private:
    void clearLayout() const;

    /// @brief Clear the panel if the shown entity no longer exists in the
    /// scene, so the editors never touch a dangling entity
    /// @return true if the shown entity is still alive
    bool validateEntity();

    Scene *m_scene = nullptr;
    CommandStack *m_commandStack = nullptr;
    Entity *m_entity = nullptr;
    EntityId m_entityId = 0;
    QVBoxLayout *m_layout;
    class PointPropertiesWidget *m_pointWidget = nullptr;
    class BezierC0Widget *m_bezierC0Widget = nullptr;
    class BezierC2Widget *m_bezierC2Widget = nullptr;
    class InterpC2Widget *m_interpC2Widget = nullptr;
};

#endif // ENTITYPROPERTIESWIDGET_H
