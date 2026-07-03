//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_INPUTMAP_HPP
#define CAD_INPUTMAP_HPP

#include <algorithm>
#include <optional>
#include <variant>
#include <QHash>
#include <QKeyCombination>
#include <QKeySequence>
#include <QList>

enum class InputAction {
    // transform mode

    beginTranslate,
    beginRotate,
    beginScale,

    constrainX,
    constrainY,
    constrainZ,

    confirmTransform,
    cancelTransform,

    // viewport

    deleteSelected,
    toggleCoordSpace,
    switchCamera,
    createMenu,

    undo,
    redo,

    setObjectSelectMode,
    setBoxSelectMode,
    selectActiveCursor,
    resetRotation,

    // camera

    cameraOrbit,
    cameraPan,
    cameraZoomDrag,
    cameraMoveUp,
    cameraMoveDown,
    cameraMoveLeft,
    cameraMoveRight,
    cameraToggleProjection,

    // viewport actions

    select,
    cursorPlace,
    rightClick,

    toggleClickToAdd,
};

struct InputBinding {
    std::variant<Qt::Key, Qt::MouseButton> input;
    Qt::KeyboardModifiers modifiers{Qt::NoModifier};
    bool allowAutoRepeat{false};
};

class InputMap final {
public:
    InputMap();

    void bind(InputAction action, const InputBinding &binding);

    /// @brief Returns the action bound to this input. 
    /// Filters out non-repeating bindings when isAutoRepeat is true
    [[nodiscard]] std::optional<InputAction> matchAction(
        Qt::Key key,
        Qt::KeyboardModifiers mods,
        bool isAutoRepeat = false
    ) const;

    /// @brief Returns the action bound to this input
    [[nodiscard]] std::optional<InputAction> matchAction(Qt::MouseButton button, Qt::KeyboardModifiers mods) const;

    /// @brief Returns every key chord bound to this action, simplest chord first.
    /// Lets menus display (and own) shortcuts sourced from the single binding table
    [[nodiscard]] QList<QKeySequence> sequencesFor(InputAction action) const;

    // TODO: Key release events should not match on modifiers, because the user may release modifier keys
    //  before releasing the main key. Currently this is not a problem because all release-sensitive bindings
    //  use NoModifier. If modifier-sensitive held-key bindings are added, consider
    //  tracking active key actions in the widget (QSet<InputAction> m_heldKeyActions) and clearing them
    //  on release without re-matching through the InputMap

private:
    struct KeyCombo {
        Qt::Key key;
        Qt::KeyboardModifiers mods;

        bool operator==(const KeyCombo &o) const {
            return key == o.key && mods == o.mods;
        }
    };

    struct ButtonCombo {
        Qt::MouseButton button;
        Qt::KeyboardModifiers mods;

        bool operator==(const ButtonCombo &o) const {
            return button == o.button && mods == o.mods;
        }
    };

    struct BoundAction {
        InputAction action;
        bool allowAutoRepeat;
    };

    friend size_t qHash(const KeyCombo &k, const size_t seed = 0) {
        return qHashMulti(seed, static_cast<int>(k.key), static_cast<int>(k.mods));
    }

    friend size_t qHash(const ButtonCombo &b, const size_t seed = 0) {
        return qHashMulti(seed, static_cast<int>(b.button), static_cast<int>(b.mods));
    }

    QHash<KeyCombo, BoundAction> m_keyBindings;
    QHash<ButtonCombo, BoundAction> m_mouseBindings;
};

inline InputMap::InputMap() {
    bind(InputAction::beginTranslate, {Qt::Key_G, Qt::NoModifier});
    bind(InputAction::beginRotate, {Qt::Key_R, Qt::NoModifier});
    bind(InputAction::beginScale, {Qt::Key_S, Qt::NoModifier});
    bind(InputAction::constrainX, {Qt::Key_X, Qt::NoModifier});
    bind(InputAction::constrainY, {Qt::Key_Y, Qt::NoModifier});
    bind(InputAction::constrainZ, {Qt::Key_Z, Qt::NoModifier});
    bind(InputAction::confirmTransform, {Qt::Key_Return, Qt::NoModifier});
    bind(InputAction::cancelTransform, {Qt::Key_Escape, Qt::NoModifier});
    bind(InputAction::deleteSelected, {Qt::Key_Delete, Qt::NoModifier});
    bind(InputAction::toggleCoordSpace, {Qt::Key_QuoteLeft, Qt::NoModifier});
    bind(InputAction::switchCamera, {Qt::Key_N, Qt::NoModifier});
    bind(InputAction::createMenu, {Qt::Key_C, Qt::NoModifier});

    bind(InputAction::undo, {Qt::Key_Z, Qt::ControlModifier, true});
    bind(InputAction::redo, {Qt::Key_Y, Qt::ControlModifier, true});
    bind(InputAction::redo, {Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier, true});

    bind(InputAction::setObjectSelectMode, {Qt::Key_O, Qt::NoModifier});
    bind(InputAction::setBoxSelectMode, {Qt::Key_B, Qt::NoModifier});
    bind(InputAction::selectActiveCursor, {Qt::Key_C, Qt::ShiftModifier});
    bind(InputAction::resetRotation, {Qt::Key_R, Qt::AltModifier});

    bind(InputAction::cameraToggleProjection, {Qt::Key_5, Qt::NoModifier});
    bind(InputAction::cameraMoveUp, {Qt::Key_Up, Qt::NoModifier, true});
    bind(InputAction::cameraMoveDown, {Qt::Key_Down, Qt::NoModifier, true});
    bind(InputAction::cameraMoveLeft, {Qt::Key_Left, Qt::NoModifier, true});
    bind(InputAction::cameraMoveRight, {Qt::Key_Right, Qt::NoModifier, true});

    bind(InputAction::cameraOrbit, {Qt::MiddleButton, Qt::NoModifier});
    bind(InputAction::cameraPan, {Qt::MiddleButton, Qt::ShiftModifier});
    bind(InputAction::cameraZoomDrag, {Qt::MiddleButton, Qt::ControlModifier});
    bind(InputAction::toggleClickToAdd, {Qt::Key_P, Qt::NoModifier});
    bind(InputAction::select, {Qt::LeftButton, Qt::NoModifier});
    bind(InputAction::cursorPlace, {Qt::LeftButton, Qt::ShiftModifier});
    bind(InputAction::rightClick, {Qt::RightButton, Qt::NoModifier});
}

inline void InputMap::bind(const InputAction action, const InputBinding &binding) {
    if (std::holds_alternative<Qt::Key>(binding.input)) {
        m_keyBindings[{std::get<Qt::Key>(binding.input), binding.modifiers}] = {action, binding.allowAutoRepeat};
    }
    else {
        m_mouseBindings[{std::get<Qt::MouseButton>(binding.input), binding.modifiers}] = {
            action,
            binding.allowAutoRepeat
        };
    }
}

inline std::optional<InputAction> InputMap::matchAction(
    const Qt::Key key,
    const Qt::KeyboardModifiers mods,
    const bool isAutoRepeat
) const {
    if (const auto it = m_keyBindings.find({key, mods});
        it != m_keyBindings.cend() && (it->allowAutoRepeat || !isAutoRepeat)) {
        return it->action;
    }
    return std::nullopt;
}

inline std::optional<InputAction> InputMap::matchAction(
    const Qt::MouseButton button,
    const Qt::KeyboardModifiers mods
) const {
    if (const auto it = m_mouseBindings.find({button, mods});
        it != m_mouseBindings.cend()) {
        return it->action;
    }
    return std::nullopt;
}

inline QList<QKeySequence> InputMap::sequencesFor(const InputAction action) const {
    QList<QKeySequence> sequences;
    for (auto it = m_keyBindings.cbegin(); it != m_keyBindings.cend(); ++it) {
        if (it->action == action) {
            sequences.append(QKeySequence(QKeyCombination(it.key().mods, it.key().key)));
        }
    }
    // deterministic order: hash iteration is unordered, so sort by chord complexity
    // (fewest modifiers first) to keep the conventional shortcut as the menu's primary
    std::ranges::sort(
        sequences,
        [](const QKeySequence &a, const QKeySequence &b) {
            return a.toString(QKeySequence::PortableText).count(QLatin1Char('+'))
                < b.toString(QKeySequence::PortableText).count(QLatin1Char('+'));
        }
    );
    return sequences;
}

#endif //CAD_INPUTMAP_HPP
