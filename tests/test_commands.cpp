#include <catch2/catch_test_macros.hpp>

#include "commands/Command.hpp"
#include "commands/CommandStack.hpp"

namespace {
    /// Test command that adds delta to a shared accumulator on executing and subtracts it on undo. 
    /// Optionally merges with another AddCommand
    class AddCommand final : public Command {
    public:
        AddCommand(int &target, const int delta, const bool mergeable = false) : m_target(target),
            m_delta(delta),
            m_mergeable(mergeable) {}

        void execute() override {
            m_target += m_delta;
        }

        void undo() override {
            m_target -= m_delta;
        }

        bool tryMerge(const Command &next) override {
            if (!m_mergeable) {
                return false;
            }
            const auto *other = dynamic_cast<const AddCommand*>(&next);
            if (!other) {
                return false;
            }
            m_delta += other->m_delta;
            return true;
        }

    private:
        int &m_target;
        int m_delta;
        bool m_mergeable;
    };
}

TEST_CASE("push executes the command", "[command]") {
    int value = 0;
    CommandStack stack;
    stack.push(std::make_unique<AddCommand>(value, 5));
    REQUIRE(value == 5);
    REQUIRE(stack.canUndo());
    REQUIRE_FALSE(stack.canRedo());
}

TEST_CASE("undo reverts and redo re-applies", "[command]") {
    int value = 0;
    CommandStack stack;
    stack.push(std::make_unique<AddCommand>(value, 5));
    stack.push(std::make_unique<AddCommand>(value, 3));
    REQUIRE(value == 8);

    stack.undo();
    REQUIRE(value == 5);
    stack.undo();
    REQUIRE(value == 0);
    REQUIRE_FALSE(stack.canUndo());
    REQUIRE(stack.canRedo());

    stack.redo();
    REQUIRE(value == 5);
    stack.redo();
    REQUIRE(value == 8);
    REQUIRE_FALSE(stack.canRedo());
}

TEST_CASE("a new push clears the redo stack", "[command]") {
    int value = 0;
    CommandStack stack;
    stack.push(std::make_unique<AddCommand>(value, 5));
    stack.undo();
    REQUIRE(stack.canRedo());

    stack.push(std::make_unique<AddCommand>(value, 100));
    REQUIRE_FALSE(stack.canRedo());
    REQUIRE(value == 100);

    // undo should only walk back through the surviving history
    stack.undo();
    REQUIRE(value == 0);
    REQUIRE_FALSE(stack.canUndo());
}

TEST_CASE("coalescing folds a gesture into one undo step", "[command]") {
    int value = 0;
    CommandStack stack;
    stack.push(std::make_unique<AddCommand>(value, 1, true), true);
    stack.push(std::make_unique<AddCommand>(value, 1, true), true);
    stack.push(std::make_unique<AddCommand>(value, 1, true), true);
    REQUIRE(value == 3);

    // all three edits collapsed into a single undoable step
    stack.undo();
    REQUIRE(value == 0);
    REQUIRE_FALSE(stack.canUndo());
}

TEST_CASE("onChange fires on push, undo and redo", "[command]") {
    int value = 0;
    int changes = 0;
    CommandStack stack;
    stack.onChange = [&changes] {
        ++changes;
    };

    stack.push(std::make_unique<AddCommand>(value, 1));
    stack.undo();
    stack.redo();
    REQUIRE(changes == 3);
}
