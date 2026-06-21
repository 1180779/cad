//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_THEME_HPP
#define CAD_THEME_HPP

#include <QApplication>
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

    // status bar

    /// @brief Magenta accent for the active transform / add-point mode (vim-like)
    inline constexpr QColor gc_statusActive(0xFF, 0x00, 0xFF);

    // frameless window-control buttons (QSS color literals, consumed by g_windowButtonStyle)

    inline constexpr auto gc_windowButtonHover = "rgba(0, 0, 0, 0.08)";
    inline constexpr auto gc_windowButtonPressed = "rgba(0, 0, 0, 0.14)";
    inline constexpr auto gc_closeButtonHover = "#E81123";
    inline constexpr auto gc_closeButtonPressed = "#C50E1F";

    /// @brief IntelliJ inspired spin box style
    /// @note Standard colors come from palette(...); only the derived tints are args
    inline const QString g_spinBoxStyle = QStringLiteral(
            R"(
                QAbstractSpinBox {
                    background-color: palette(base);
                    border: 1px solid %1;
                    border-radius: %2px;
                    padding: 2px 6px;
                    selection-background-color: palette(highlight);
                    selection-color: palette(highlighted-text);
                }
                QAbstractSpinBox:focus { border: 1px solid palette(highlight); }
                QAbstractSpinBox:disabled { color: %3; background-color: palette(window); }
                QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
                    subcontrol-origin: border;
                    width: 15px;
                    border-left: 1px solid %1;
                    background-color: transparent;
                }
                QAbstractSpinBox::up-button { subcontrol-position: top right; border-top-right-radius: %2px; }
                QAbstractSpinBox::down-button { subcontrol-position: bottom right; border-bottom-right-radius: %2px; }
                QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background-color: %4; }
                QAbstractSpinBox::up-arrow { image: url(:/icons/chevron-up.svg); width: 9px; height: 9px; }
                QAbstractSpinBox::down-arrow { image: url(:/icons/chevron-down.svg); width: 9px; height: 9px; }
                QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::down-arrow:disabled { image: none; }
            )"
        )
        .arg(
            g_inputBorder.name(),
            QString::number(gc_inputRadius),
            g_menuDisabled.name(),
            g_menuHover.name()
        );

    /// @brief IntelliJ inspired combo box style
    /// @note Standard colors come from palette(...); only the derived tints are args
    inline const QString g_comboBoxStyle = QStringLiteral(
            R"(
                QComboBox {
                    background-color: palette(base);
                    border: 1px solid %1;
                    border-radius: %2px;
                    padding: 2px 6px;
                }
                QComboBox:focus, QComboBox:on { border: 1px solid palette(highlight); }
                QComboBox:disabled { color: %3; background-color: palette(window); }
                QComboBox::drop-down {
                    subcontrol-origin: border;
                    subcontrol-position: center right;
                    width: 18px;
                    border-left: 1px solid %1;
                    border-top-right-radius: %2px;
                    border-bottom-right-radius: %2px;
                    background-color: transparent;
                }
                QComboBox::drop-down:hover { background-color: %4; }
                QComboBox::down-arrow { image: url(:/icons/chevron-down.svg); width: 9px; height: 9px; }
                QComboBox::down-arrow:disabled { image: none; }
                QComboBox QAbstractItemView {
                    background-color: palette(base);
                    border: 1px solid %5;
                    border-radius: %6px;
                    padding: 4px;
                    outline: 0;
                }
                QComboBox QAbstractItemView::item { padding: 4px 8px; border-radius: %6px; }
                QComboBox QAbstractItemView::item:selected {
                    background-color: palette(highlight);
                    color: palette(highlighted-text);
                }
            )"
        )
        .arg(
            g_inputBorder.name(),
            QString::number(gc_inputRadius),
            g_menuDisabled.name(),
            g_menuHover.name(),
            g_menuBorder.name(),
            QString::number(gc_itemRadius)
        );

    /// @brief IntelliJ inspired menu bar and menus style
    /// @note Standard colors come from palette(...); only the derived tints are args
    inline const QString g_menuStyle = QStringLiteral(
            R"(
                QMenuBar { background: transparent; }
                QMenuBar::item { padding: 4px 8px; background: transparent; border-radius: %4px; }
                QMenuBar::item:selected { background: %1; }
                QMenuBar::item:pressed { background: %1; }
                QMenu { background-color: palette(window); border: 1px solid %2; padding: 4px; }
                QMenu::item { padding: 4px 24px; border-radius: %4px; }
                QMenu::item:selected { background-color: palette(highlight); color: palette(highlighted-text); }
                QMenu::item:disabled { color: %3; }
                QMenu::separator { height: 1px; background: %2; margin: 4px 8px; }
            )"
        )
                                       .arg(
                                           g_menuHover.name(),
                                           g_menuBorder.name(),
                                           g_menuDisabled.name()
                                       )
                                       .arg(gc_itemRadius);

    /// @brief IntelliJ inspired style for tool windows
    /// @details Targets widgets named "toolPanel"; 
    /// each panel must also set that objectName + WA_StyledBackground
    inline const QString g_toolPanelStyle = QStringLiteral(
            "#toolPanel { background-color: palette(base); border-radius: %1px; }"
        )
        .arg(gc_cardRadius);

    /// @brief Vim inspired status bar style
    inline const QString g_statusBarStyle = QStringLiteral(
            R"(
                StatusBarWidget { background-color: palette(base); font-family: monospace; font-size: 12px; }
                StatusBarWidget QLabel { color: palette(text); }
                StatusBarWidget QLabel[modeActive="true"] { color: %1; font-weight: bold; }
            )"
        )
        .arg(gc_statusActive.name());

    /// @brief App-colored strip the splitter draws between the viewport and tool window
    inline const auto g_splitterStyle = QStringLiteral(
        "QSplitter::handle { background-color: palette(window); }"
    );

    /// @brief Frameless window controls (objectName "windowButton"); the close button
    /// (the "close" dynamic property) gets the red hover / pressed accents
    inline const QString g_windowButtonStyle = QStringLiteral(
            R"(
                QToolButton#windowButton { background: transparent; border: none; }
                QToolButton#windowButton:hover { background: %1; }
                QToolButton#windowButton:pressed { background: %2; }
                QToolButton#windowButton[close="true"]:hover { background: %3; }
                QToolButton#windowButton[close="true"]:pressed { background: %4; }
            )"
        )
        .arg(
            QLatin1String(gc_windowButtonHover),
            QLatin1String(gc_windowButtonPressed),
            QLatin1String(gc_closeButtonHover),
            QLatin1String(gc_closeButtonPressed)
        );

    /// @brief Full application stylesheet
    inline QString appStyleSheet() {
        return g_spinBoxStyle + g_comboBoxStyle + g_menuStyle + g_toolPanelStyle
            + g_statusBarStyle + g_splitterStyle + g_windowButtonStyle;
    }

    /// @brief Overlays the theme's colors onto the standard roles of an existing palette,
    /// leaving every other role at the style's defaults
    inline QPalette applyTheme(QPalette palette) {
        palette.setColor(QPalette::Window, gc_appBackground);
        palette.setColor(QPalette::Base, gc_cardBackground);
        palette.setColor(QPalette::Highlight, gc_accent);
        palette.setColor(QPalette::HighlightedText, QColor(Qt::white));
        return palette;
    }

    /// @brief Applies the full theme to the running application
    inline void apply() {
        QApplication::setPalette(applyTheme(QApplication::palette()));
        qApp->setStyleSheet(appStyleSheet());
    }
}

#endif //CAD_THEME_HPP
