//
// Created on 6/19/26.
//

#ifndef CAD_COMMAND_HPP
#define CAD_COMMAND_HPP

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
};

#endif //CAD_COMMAND_HPP
