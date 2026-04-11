#pragma once

#include <optional>
#include <variant>
#include <QHash>
#include <Qt>

enum class InputAction
{
    // Transform mode
    BeginTranslate,
    BeginRotate,
    BeginScale,

    ConstrainX,
    ConstrainY,
    ConstrainZ,

    ConfirmTransform,
    CancelTransform,

    // Viewport
    DeleteSelected,
    ToggleCoordSpace,
    SwitchCamera,
    CreateMenu,

    SetObjectSelectMode,
    SetBoxSelectMode,

    // Camera
    CameraOrbit,
    CameraPan,
    CameraZoomDrag,
    CameraMoveUp,
    CameraMoveDown,
    CameraMoveLeft,
    CameraMoveRight,

    // Viewport actions
    Select,
    CursorPlace,
    RightClick,
};

struct InputBinding
{
    std::variant<Qt::Key, Qt::MouseButton> input;
    Qt::KeyboardModifiers modifiers{Qt::NoModifier};
    bool allowAutoRepeat{false};
};

class InputMap
{
public:
    InputMap();

    void bind(InputAction action, const InputBinding &binding);

    // Returns the action bound to this input. Filters out non-repeating bindings when isAutoRepeat is true.
    [[nodiscard]] std::optional<InputAction> matchAction(
        Qt::Key key,
        Qt::KeyboardModifiers mods,
        bool isAutoRepeat = false) const;

    // Returns the action bound to this input.
    [[nodiscard]] std::optional<InputAction> matchAction(Qt::MouseButton button, Qt::KeyboardModifiers mods) const;

    // TODO: Key release events should not match on modifiers, because the user may release modifier keys
    //  before releasing the main key. Currently this is not a problem because all release-sensitive bindings
    //  use NoModifier. If modifier-sensitive held-key bindings are added, consider
    //  tracking active key actions in the widget (QSet<InputAction> m_heldKeyActions) and clearing them
    //  on release without re-matching through the InputMap.

private:
    struct KeyCombo
    {
        Qt::Key key;
        Qt::KeyboardModifiers mods;
        bool operator==(const KeyCombo &o) const { return key == o.key && mods == o.mods; }
    };

    struct ButtonCombo
    {
        Qt::MouseButton button;
        Qt::KeyboardModifiers mods;
        bool operator==(const ButtonCombo &o) const { return button == o.button && mods == o.mods; }
    };

    struct BoundAction
    {
        InputAction action;
        bool allowAutoRepeat;
    };

    friend size_t qHash(const KeyCombo &k, const size_t seed = 0)
    {
        return qHashMulti(seed, static_cast<int>(k.key), static_cast<int>(k.mods));
    }

    friend size_t qHash(const ButtonCombo &b, const size_t seed = 0)
    {
        return qHashMulti(seed, static_cast<int>(b.button), static_cast<int>(b.mods));
    }

    QHash<KeyCombo, BoundAction> m_keyBindings;
    QHash<ButtonCombo, BoundAction> m_mouseBindings;
};

inline InputMap::InputMap()
{
    bind(InputAction::BeginTranslate, {Qt::Key_G, Qt::NoModifier});
    bind(InputAction::BeginRotate, {Qt::Key_R, Qt::NoModifier});
    bind(InputAction::BeginScale, {Qt::Key_S, Qt::NoModifier});
    bind(InputAction::ConstrainX, {Qt::Key_X, Qt::NoModifier});
    bind(InputAction::ConstrainY, {Qt::Key_Y, Qt::NoModifier});
    bind(InputAction::ConstrainZ, {Qt::Key_Z, Qt::NoModifier});
    bind(InputAction::ConfirmTransform, {Qt::Key_Return, Qt::NoModifier});
    bind(InputAction::CancelTransform, {Qt::Key_Escape, Qt::NoModifier});
    bind(InputAction::DeleteSelected, {Qt::Key_Delete, Qt::NoModifier});
    bind(InputAction::ToggleCoordSpace, {Qt::Key_QuoteLeft, Qt::NoModifier});
    bind(InputAction::SwitchCamera, {Qt::Key_N, Qt::NoModifier});
    bind(InputAction::CreateMenu, {Qt::Key_C, Qt::NoModifier});

    bind(InputAction::SetObjectSelectMode, {Qt::Key_O, Qt::NoModifier});
    bind(InputAction::SetBoxSelectMode, {Qt::Key_B, Qt::NoModifier});

    bind(InputAction::CameraMoveUp, {Qt::Key_Up, Qt::NoModifier, true});
    bind(InputAction::CameraMoveDown, {Qt::Key_Down, Qt::NoModifier, true});
    bind(InputAction::CameraMoveLeft, {Qt::Key_Left, Qt::NoModifier, true});
    bind(InputAction::CameraMoveRight, {Qt::Key_Right, Qt::NoModifier, true});

    bind(InputAction::CameraOrbit, {Qt::MiddleButton, Qt::NoModifier});
    bind(InputAction::CameraPan, {Qt::MiddleButton, Qt::ShiftModifier});
    bind(InputAction::CameraZoomDrag, {Qt::MiddleButton, Qt::ControlModifier});
    bind(InputAction::Select, {Qt::LeftButton, Qt::NoModifier});
    bind(InputAction::CursorPlace, {Qt::LeftButton, Qt::ShiftModifier});
    bind(InputAction::RightClick, {Qt::RightButton, Qt::NoModifier});
}

inline void InputMap::bind(const InputAction action, const InputBinding &binding)
{
    if (std::holds_alternative<Qt::Key>(binding.input))
        m_keyBindings[{std::get<Qt::Key>(binding.input), binding.modifiers}] = {action, binding.allowAutoRepeat};
    else
        m_mouseBindings[{std::get<Qt::MouseButton>(binding.input), binding.modifiers}] = {
            action,
            binding.allowAutoRepeat
        };
}

inline std::optional<InputAction> InputMap::matchAction(
    const Qt::Key key,
    const Qt::KeyboardModifiers mods,
    const bool isAutoRepeat) const
{
    if (const auto it = m_keyBindings.find({key, mods});
        it != m_keyBindings.cend() && (it->allowAutoRepeat || !isAutoRepeat))
        return it->action;
    return std::nullopt;
}

inline std::optional<InputAction> InputMap::matchAction(
    const Qt::MouseButton button,
    const Qt::KeyboardModifiers mods) const
{
    if (const auto it = m_mouseBindings.find({button, mods});
        it != m_mouseBindings.cend())
        return it->action;
    return std::nullopt;
}
