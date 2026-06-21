//
// Created on 6/19/26.
//

#ifndef CAD_COMMAND_HPP
#define CAD_COMMAND_HPP

#include <cstdint>

/// @brief What a command changed, so the UI refreshes only what is affected
enum class ChangeFlags : std::uint32_t {
    none = 0,

    /// @brief Values moved/edited
    geometry = 1u << 0,

    /// @brief The set of selected entities changed
    selection = 1u << 1,

    /// @brief Entities added/removed/renamed
    structure = 1u << 2,
    all = geometry | selection | structure
};

constexpr ChangeFlags operator|(const ChangeFlags a, const ChangeFlags b) {
    return static_cast<ChangeFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr bool hasFlag(const ChangeFlags set, const ChangeFlags flag) {
    return (static_cast<std::uint32_t>(set) & static_cast<std::uint32_t>(flag)) != 0;
}

/// @brief
/// A reversible scene mutation.
/// execute() applies the change (also used for redo);
/// undo() reverts it. 
/// Commands call Scene mutators only and never the UI directly
class Command {
public:
    virtual ~Command() = default;

    virtual void execute() = 0;

    virtual void undo() = 0;

    /// @brief 
    /// Attempt to fold next into this command so a continuous gesture 
    /// (a drag, a spinbox scrub) collapses into a single undo step. 
    /// Return true if merged; the stack then discards next. 
    /// @return True if the commands were merged, false otherwise
    /// @note Default: no merging
    virtual bool tryMerge(const Command &next) {
        return false;
    }

    /// @brief Which parts of the scene this command changes, so the UI refreshes only
    /// those. Defaults to everything; derived commands narrow it accordingly
    [[nodiscard]] virtual ChangeFlags changeFlags() const {
        return ChangeFlags::all;
    }
};

#endif //CAD_COMMAND_HPP
