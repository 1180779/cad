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

    // Viewport actions
    Select,
    CursorPlace,
    RightClick,
};

enum class InputTrigger
{
    // fires once on key/button-down
    OnPress,
    // fires on both press and release (caller handles both ends)
    WhileHeld
};

struct InputBinding
{
    std::variant<Qt::Key, Qt::MouseButton> input;
    Qt::KeyboardModifiers modifiers{Qt::NoModifier};
    InputTrigger trigger{InputTrigger::OnPress};
};

class InputMap
{
public:
    InputMap();

    void bind(InputAction action, InputBinding binding);

    // Returns the action bound to this input
    [[nodiscard]] std::optional<InputAction> matchAction(Qt::Key key, Qt::KeyboardModifiers mods) const;
    // Returns the action bound to this input
    [[nodiscard]] std::optional<InputAction> matchAction(Qt::MouseButton button, Qt::KeyboardModifiers mods) const;

    // Returns the action only for WhileHeld bindings (for drag/held-key release).
    [[nodiscard]] std::optional<InputAction> matchRelease(Qt::Key key, Qt::KeyboardModifiers mods) const;
    // Returns the action only for WhileHeld bindings (for drag/held-key release).
    [[nodiscard]] std::optional<InputAction> matchRelease(Qt::MouseButton button, Qt::KeyboardModifiers mods) const;

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
        InputTrigger trigger;
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
    bind(InputAction::BeginTranslate, {Qt::Key_G, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::BeginRotate, {Qt::Key_R, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::BeginScale, {Qt::Key_S, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::ConstrainX, {Qt::Key_X, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::ConstrainY, {Qt::Key_Y, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::ConstrainZ, {Qt::Key_Z, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::ConfirmTransform, {Qt::Key_Return, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::CancelTransform, {Qt::Key_Escape, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::DeleteSelected, {Qt::Key_Delete, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::ToggleCoordSpace, {Qt::Key_QuoteLeft, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::SwitchCamera, {Qt::Key_N, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::CreateMenu, {Qt::Key_C, Qt::NoModifier, InputTrigger::OnPress});

    bind(InputAction::SetObjectSelectMode, {Qt::Key_O, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::SetBoxSelectMode, {Qt::Key_B, Qt::NoModifier, InputTrigger::OnPress});

    bind(InputAction::CameraOrbit, {Qt::MiddleButton, Qt::NoModifier, InputTrigger::WhileHeld});
    bind(InputAction::CameraPan, {Qt::MiddleButton, Qt::ShiftModifier, InputTrigger::WhileHeld});
    bind(InputAction::CameraZoomDrag, {Qt::MiddleButton, Qt::ControlModifier, InputTrigger::WhileHeld});
    bind(InputAction::Select, {Qt::LeftButton, Qt::NoModifier, InputTrigger::OnPress});
    bind(InputAction::CursorPlace, {Qt::LeftButton, Qt::ShiftModifier, InputTrigger::WhileHeld});
    bind(InputAction::RightClick, {Qt::RightButton, Qt::NoModifier, InputTrigger::OnPress});
}

inline void InputMap::bind(const InputAction action, const InputBinding binding)
{
    if (std::holds_alternative<Qt::Key>(binding.input))
        m_keyBindings[{std::get<Qt::Key>(binding.input), binding.modifiers}] = {action, binding.trigger};
    else
        m_mouseBindings[{std::get<Qt::MouseButton>(binding.input), binding.modifiers}] = {action, binding.trigger};
}

inline std::optional<InputAction> InputMap::matchAction(const Qt::Key key, const Qt::KeyboardModifiers mods) const
{
    if (const auto it = m_keyBindings.find({key, mods});
        it != m_keyBindings.cend())
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


inline std::optional<InputAction> InputMap::matchRelease(const Qt::Key key, const Qt::KeyboardModifiers mods) const
{
    if (const auto it = m_keyBindings.find({key, mods});
        it != m_keyBindings.cend() && it->trigger == InputTrigger::WhileHeld)
        return it->action;
    return std::nullopt;
}

inline std::optional<InputAction> InputMap::matchRelease(
    const Qt::MouseButton button,
    const Qt::KeyboardModifiers mods) const
{
    if (const auto it = m_mouseBindings.find({button, mods});
        it != m_mouseBindings.cend() && it->trigger == InputTrigger::WhileHeld)
        return it->action;
    return std::nullopt;
}
