//
// Created on 6/19/26.
//

#ifndef CAD_COMMANDSTACK_HPP
#define CAD_COMMANDSTACK_HPP

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>

#include "Command.hpp"

/// @brief
/// Bounded undo/redo history.
/// undo()/redo() move commands between the two deques. A fresh push clears the redo
/// deque, and once the undo history exceeds the capacity the oldest entry is dropped
/// from the front in O(1).
///
/// TODO (low priority): swap the two deques for a single fixed-capacity ring buffer
///     (no noticeable performance gain)
class CommandStack {
public:
    /// @brief Default number of undo steps retained before the oldest is dropped
    static constexpr std::size_t gc_defaultCapacity = 1024;

    /// @brief Construct with a fixed undo-history capacity (clamped to at least 1)
    explicit CommandStack(std::size_t capacity = gc_defaultCapacity);

    /// @brief Executes a command and pushes it onto the undo history, clearing redo
    /// history. If coalesce and the previous command accepts a merge, the two collapse
    /// into a single undo step. When the history is full, the oldest entry is dropped
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

    /// @brief Maximum number of retained undo steps; older entries are evicted
    std::size_t m_capacity;

    std::deque<std::unique_ptr<Command>> m_undo;
    std::deque<std::unique_ptr<Command>> m_redo;
};

#endif //CAD_COMMANDSTACK_HPP
