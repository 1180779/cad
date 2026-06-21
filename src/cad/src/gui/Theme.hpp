//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_THEME_HPP
#define CAD_THEME_HPP

#include <QColor>
#include <QPalette>
#include <QString>

/// @brief Centralized custom theme palette
namespace theme {
    /// @brief Soft off-white app background, distinct from the white tool-window cards
    inline constexpr QColor gc_appBackground(0xF2, 0xF3, 0xF5);

    /// @brief White tool-window card base (the QPalette::Base role)
    inline constexpr QColor gc_cardBackground(0xFF, 0xFF, 0xFF);

    /// @brief Selection / highlight accent (the QPalette::Highlight role)
    inline constexpr QColor gc_accent(0x56, 0xA0, 0xEB); // (0x34, 0x74, 0xF0);

    // corner rounding scale (px)

    /// @brief Tool-window cards
    inline constexpr int gc_cardRadius = 6;

    /// @brief Menu / hover items
    inline constexpr int gc_itemRadius = 3;

    // menu theming tints, derived from the app background (darker() is not constexpr)

    /// @brief Subtle gray wash on hover
    inline const QColor g_menuHover = gc_appBackground.darker(112);

    /// @brief Muted, tinted from app bg
    inline const QColor g_menuDisabled = gc_appBackground.darker(140);

    /// @brief Dropdown border + separators
    inline const QColor g_menuBorder = gc_appBackground.darker(125);

    // text inputs / spinboxes

    /// @brief Corner rounding for text inputs and spinboxes
    inline constexpr int gc_inputRadius = 4;

    /// @brief Resting 1px border around inputs (tinted from the app bg)
    inline const QColor g_inputBorder = gc_appBackground.darker(125);

    /// @brief Flat, IntelliJ-like coordinate spinbox: soft border + rounded corners,
    /// accent border on focus, and slim themed up/down steppers with a hover tint.
    /// Set on a parent widget so it cascades to its child QAbstractSpinBoxes
    inline const QString g_spinBoxStyle = QStringLiteral(
            R"(
                QAbstractSpinBox {
                    background-color: %1;
                    border: 1px solid %2;
                    border-radius: %3px;
                    padding: 2px 6px;
                    selection-background-color: %4;
                    selection-color: white;
                }
                QAbstractSpinBox:focus { border: 1px solid %4; }
                QAbstractSpinBox:disabled { color: %5; background-color: %6; }
                QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
                    subcontrol-origin: border;
                    width: 15px;
                    border-left: 1px solid %2;
                    background-color: transparent;
                }
                QAbstractSpinBox::up-button { subcontrol-position: top right; border-top-right-radius: %3px; }
                QAbstractSpinBox::down-button { subcontrol-position: bottom right; border-bottom-right-radius: %3px; }
                QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background-color: %7; }
                QAbstractSpinBox::up-arrow { image: url(:/icons/chevron-up.svg); width: 9px; height: 9px; }
                QAbstractSpinBox::down-arrow { image: url(:/icons/chevron-down.svg); width: 9px; height: 9px; }
                QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::down-arrow:disabled { image: none; }
            )"
        )
        .arg(
            gc_cardBackground.name(),
            g_inputBorder.name(),
            QString::number(gc_inputRadius),
            gc_accent.name(),
            g_menuDisabled.name(),
            gc_appBackground.name(),
            g_menuHover.name()
        );

    // frameless window-control buttons (QSS color literals)

    inline constexpr auto gc_windowButtonHover = "rgba(0, 0, 0, 0.08)";
    inline constexpr auto gc_windowButtonPressed = "rgba(0, 0, 0, 0.14)";
    inline constexpr auto gc_closeButtonHover = "#E81123";
    inline constexpr auto gc_closeButtonPressed = "#C50E1F";

    /// @brief Overlays the theme's colors onto the standard roles of an existing palette,
    /// leaving every other role at the style's defaults
    inline QPalette applyTheme(QPalette palette) {
        palette.setColor(QPalette::Window, gc_appBackground);
        palette.setColor(QPalette::Base, gc_cardBackground);
        palette.setColor(QPalette::Highlight, gc_accent);
        palette.setColor(QPalette::HighlightedText, QColor(Qt::white));
        return palette;
    }
}

#endif //CAD_THEME_HPP
