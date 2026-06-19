//
// Created on 6/19/26.
//

#ifndef CAD_COMMANDSTACK_HPP
#define CAD_COMMANDSTACK_HPP

#include <functional>
#include <memory>
#include <vector>

#include "Command.hpp"

/// @brief 
/// Undo/redo history. 
/// undo()/redo() move between the two stacks. A fresh push clears the redo stack.
///
/// Kept free of Qt, so it is unit-testable in isolation; 
/// callers hook onChange to refresh the UI
class CommandStack {
public:
    /// @brief Executes a command and pushes it onto the undo stack, clearing redo history.
    /// If coalesce and the previous command accepts a merge, the two collapse
    /// into a single undo step
    /// @param cmd The command to execute and store
    /// @param coalesce Whether to attempt merging this command with the previous one
    void push(std::unique_ptr<Command> cmd, bool coalesce = false);

    void undo();

    void redo();

    [[nodiscard]] bool canUndo() const {
        return !m_undo.empty();
    }

    [[nodiscard]] bool canRedo() const {
        return !m_redo.empty();
    }

    void clear();

    /// @brief Called after any push/undo/redo that changed state, so the UI can refresh
    std::function<void()> onChange;

private:
    void notify() const {
        if (onChange) {
            onChange();
        }
    }

    std::vector<std::unique_ptr<Command>> m_undo;
    std::vector<std::unique_ptr<Command>> m_redo;
};

#endif //CAD_COMMANDSTACK_HPP
