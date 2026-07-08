#include "ComponentWidget.hpp"

#include "../../commands/CommandStack.hpp"
#include "../../commands/Commands.hpp"

ComponentWidget::ComponentWidget(Component *component, QWidget *parent) : QWidget(parent), m_component(component) {}

void ComponentWidget::pushEdit(
    std::function<void()> apply,
    std::function<void()> revert,
    const void *mergeKey,
    const bool coalesce
) {
    if (m_commandStack) {
        m_commandStack->push(
            std::make_unique<SetPropertyCommand>(std::move(apply), std::move(revert), mergeKey),
            coalesce
        );
    }
    else {
        apply();
    }
    emit propertyChanged();
}
