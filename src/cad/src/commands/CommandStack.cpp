//
// Created on 6/19/26.
//

#include "CommandStack.hpp"

void CommandStack::push(std::unique_ptr<Command> cmd, const bool coalesce) {
    cmd->execute();

    if (coalesce && !m_undo.empty() && m_undo.back()->tryMerge(*cmd)) {
        m_redo.clear();
        notify();
        return;
    }

    m_undo.push_back(std::move(cmd));
    m_redo.clear();
    notify();
}

void CommandStack::undo() {
    if (m_undo.empty()) {
        return;
    }
    auto cmd = std::move(m_undo.back());
    m_undo.pop_back();
    cmd->undo();
    m_redo.push_back(std::move(cmd));
    notify();
}

void CommandStack::redo() {
    if (m_redo.empty()) {
        return;
    }
    auto cmd = std::move(m_redo.back());
    m_redo.pop_back();
    cmd->execute();
    m_undo.push_back(std::move(cmd));
    notify();
}

void CommandStack::clear() {
    m_undo.clear();
    m_redo.clear();
}
