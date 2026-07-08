#ifndef COMPONENTWIDGET_H
#define COMPONENTWIDGET_H

#include <QWidget>
#include <functional>

#include "../../components/Entity.hpp"

class CommandStack;
class Scene;

class ComponentWidget : public QWidget {
    Q_OBJECT

public:
    static constexpr int s_doubleSpinBoxFixedWidth = 80;

    explicit ComponentWidget(Component *component, QWidget *parent = nullptr);

    /// @brief Wire the command stack (and owning entity id) so edits become undoable
    /// @param scene The scene containing the entity
    /// @param stack The command stack to push edits to
    /// @param entityId The ID of the entity owning the component
    virtual void setCommandContext(
        Scene *scene,
        CommandStack *stack,
        const EntityId entityId
    ) {
        m_scene = scene;
        m_commandStack = stack;
        m_entityId = entityId;
    }

signals :
    void propertyChanged();

protected:
    /// @brief Records a property edit as a command on the stack.
    /// If a command stack is present, it pushes a new command (optionally coalesced by mergeKey).
    /// If no stack is present, it executes the apply function directly
    /// @param apply Function to apply the change
    /// @param revert Function to undo the change
    /// @param mergeKey Pointer used to identify and merge consecutive related commands
    /// @param coalesce Whether to attempt merging this command with the previous one on the stack
    void pushEdit(
        std::function<void()> apply,
        std::function<void()> revert,
        const void *mergeKey,
        bool coalesce = false
    );

    /// @brief Build an undoable handler for a bool property toggle
    /// @details Returns a <code>void(bool)</code> slot that pushes a setter/un-setter pair, 
    /// coalesced by @p mergeKey 
    /// @param component the component to mutate
    /// @param setter the bool setter to invoke
    /// @param mergeKey coalescing key (typically the source widget)
    /// @note Lets a constructor wire toggles as plain one-line connects: <br>
    /// <code>connect(box, &QCheckBox::toggled, this, makeBoolToggle(comp, &Comp::setFoo, box));</code>
    template <class C>
    auto makeBoolToggle(C *component, void (C::*setter)(bool), const void *mergeKey) {
        return [this, component, setter, mergeKey](const bool checked) {
            pushEdit(
                [component, setter, checked] {
                    (component->*setter)(checked);
                },
                [component, setter, checked] {
                    (component->*setter)(!checked);
                },
                mergeKey,
                false
            );
        };
    }

    Component *m_component;
    Scene *m_scene = nullptr;
    CommandStack *m_commandStack = nullptr;
    EntityId m_entityId = 0;
};

#endif // COMPONENTWIDGET_H
