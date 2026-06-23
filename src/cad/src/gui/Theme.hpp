//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_THEME_HPP
#define CAD_THEME_HPP

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QString>
#include <QStyle>
#include <QTemporaryDir>

/// @brief Centralized custom theme palette
namespace theme {
    /// @brief A single theme's seed colors. Everything else (tints, QSS) derives from these.
    /// @note ponytail: phase 1 keeps light+dark here; phase 2 lifts the generic builders
    /// (spin/combo/menu) into a reusable submodule, leaving app overrides behind.
    struct ThemeColors {
        QColor window; ///< app background (QPalette::Window)
        QColor base; ///< tool-window card / input base (QPalette::Base)
        QColor accent; ///< selection / highlight (QPalette::Highlight)
        QColor text; ///< primary foreground
        QColor viewport; ///< OpenGL clear color for the non-stereo viewport
        // viewport geometry colors (uploaded to the Palette UBO, binding 1)
        QColor line; ///< basic geometry lines
        QColor point; ///< unselected control points
        QColor curve; ///< tessellated bezier curves
        QColor gridMinor; ///< infinite grid minor lines
        QColor gridMajor; ///< infinite grid major lines
        bool dark; ///< flips derived tints between darker()/lighter()
    };

    /// @brief Soft off-white background, white cards, dark text
    inline constexpr ThemeColors light{
        QColor(0xF2, 0xF3, 0xF5),
        QColor(0xFF, 0xFF, 0xFF),
        QColor(0x56, 0xA0, 0xEB),
        QColor(0x1A, 0x1A, 0x1A),
        QColor(0xFF, 0xFF, 0xFF),
        QColor(0x00, 0x00, 0x00),
        // line
        QColor(0x14, 0x14, 0x14),
        // point (~0.08)
        QColor(0x80, 0x80, 0x80),
        // curve (~0.5)
        QColor(0xB8, 0xB8, 0xB8),
        // gridMinor (~0.72)
        QColor(0x80, 0x80, 0x80),
        // gridMajor (~0.5)
        false
    };

    /// @brief IntelliJ-Darcula-ish: dark background, slightly darker cards, light text
    inline constexpr ThemeColors dark{
        QColor(0x2B, 0x2D, 0x30),
        QColor(0x1E, 0x1F, 0x22),
        QColor(0x56, 0xA0, 0xEB),
        QColor(0xDF, 0xE1, 0xE5),
        QColor(0x1E, 0x1F, 0x22),
        QColor(0xD7, 0xD9, 0xDD),
        // line
        QColor(0xE0, 0xE2, 0xE6),
        // point
        QColor(0xA8, 0xAB, 0xB0),
        // curve
        QColor(0x4A, 0x4D, 0x52),
        // gridMinor
        QColor(0x6A, 0x6E, 0x74),
        // gridMajor
        true
    };

    // corner rounding scale (px)

    /// @brief Tool-window cards
    inline constexpr int gc_cardRadius = 6;

    /// @brief Menu / hover items
    inline constexpr int gc_itemRadius = 3;

    /// @brief Corner rounding for text inputs and spinboxes
    inline constexpr int gc_inputRadius = 4;

    /// @brief Magenta accent for the active transform / add-point mode (vim-like)
    inline constexpr QColor gc_statusActive(0xFF, 0x00, 0xFF);

    // ---- derived tints (theme-relative: darken light bg, lighten dark bg) ----

    /// @brief Shifts the window color toward contrast by @p amount (Qt darker/lighter scale, 100 = no-op)
    inline QColor tint(const ThemeColors &t, const int amount) {
        return t.dark
                   ? t.window.lighter(amount)
                   : t.window.darker(amount);
    }

    inline QColor menuHover(const ThemeColors &t) {
        return tint(t, 112);
    } ///< subtle hover wash
    /// @brief Muted text; dark themes need a bigger lift off the bg to stay legible
    inline QColor menuDisabled(const ThemeColors &t) {
        return tint(
            t,
            t.dark
                ? 185
                : 140
        );
    }

    inline QColor menuBorder(const ThemeColors &t) {
        return tint(t, 125);
    } ///< dropdown border / separators
    inline QColor inputBorder(const ThemeColors &t) {
        return tint(t, 125);
    } ///< resting input border

    /// @brief Translucent overlay for frameless window-control hover (black on light, white on dark)
    inline QString windowButtonOverlay(const ThemeColors &t, const double a) {
        return t.dark
                   ? QStringLiteral("rgba(255, 255, 255, %1)").arg(a)
                   : QStringLiteral("rgba(0, 0, 0, %1)").arg(a);
    }

    /// @brief Recolors a `currentColor` SVG resource to @p color and returns a file path for QSS url().
    /// @details The SVGs use fill="currentColor" which QSvgRenderer resolves to black, so they vanish
    /// on dark backgrounds. We substitute the literal color and cache one file per (name, color) in a
    /// process-lifetime temp dir. ponytail: text substitution dodges linking Qt6::Svg just to recolor.
    /// @param name resource basename under :/icons/ without extension (e.g. "chevron-up", "check")
    inline QString recoloredIcon(const QString &name, const QColor &color) {
        static QTemporaryDir dir;
        const QString src = QStringLiteral(":/icons/%1.svg").arg(name);
        if (!dir.isValid()) {
            return src; // fall back to the (dark) original rather than no icon
        }
        const QString out = dir.filePath(QStringLiteral("%1-%2.svg").arg(name, color.name().mid(1)));
        if (!QFileInfo::exists(out)) {
            QFile in(src);
            if (in.open(QIODevice::ReadOnly)) {
                QByteArray svg = in.readAll();
                svg.replace("currentColor", color.name().toUtf8());
                if (QFile f(out);
                    f.open(QIODevice::WriteOnly)) {
                    f.write(svg);
                }
            }
        }
        return out;
    }

    // ==== generic, reusable QSS (palette()-driven; only derived tints are args) ====

    /// @brief IntelliJ inspired spin box style
    inline QString spinBoxStyle(const ThemeColors &t) {
        return QStringLiteral(
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
                QAbstractSpinBox::up-arrow { image: url(%5); width: 9px; height: 9px; }
                QAbstractSpinBox::down-arrow { image: url(%6); width: 9px; height: 9px; }
                QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::down-arrow:disabled { image: none; }
            )"
            )
            .arg(
                inputBorder(t).name(),
                QString::number(gc_inputRadius),
                menuDisabled(t).name(),
                menuHover(t).name(),
                recoloredIcon("chevron-up", t.text),
                recoloredIcon("chevron-down", t.text)
            );
    }

    /// @brief IntelliJ inspired combo box style
    inline QString comboBoxStyle(const ThemeColors &t) {
        return QStringLiteral(
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
                QComboBox::down-arrow { image: url(%7); width: 9px; height: 9px; }
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
                inputBorder(t).name(),
                QString::number(gc_inputRadius),
                menuDisabled(t).name(),
                menuHover(t).name(),
                menuBorder(t).name(),
                QString::number(gc_itemRadius),
                recoloredIcon("chevron-down", t.text)
            );
    }

    /// @brief IntelliJ inspired check box style
    inline QString checkBoxStyle(const ThemeColors &t) {
        return QStringLiteral(
                R"(
                QCheckBox { spacing: 6px; }
                QCheckBox:disabled { color: %4; }
                QCheckBox::indicator {
                    width: 14px; height: 14px;
                    border: 1px solid %1;
                    border-radius: %2px;
                    background-color: palette(base);
                }
                QCheckBox::indicator:hover { border-color: palette(highlight); }
                QCheckBox::indicator:checked {
                    background-color: palette(highlight);
                    border-color: palette(highlight);
                    image: url(%3);
                }
                QCheckBox::indicator:disabled { background-color: palette(window); border-color: %5; }
            )"
            )
            .arg(
                inputBorder(t).name(),
                QString::number(gc_inputRadius),
                recoloredIcon("check", QColor(Qt::white)),
                menuDisabled(t).name(),
                menuBorder(t).name()
            );
    }

    /// @brief IntelliJ inspired push button style
    inline QString pushButtonStyle(const ThemeColors &t) {
        return QStringLiteral(
                R"(
                QPushButton {
                    background-color: palette(base);
                    border: 1px solid %1;
                    border-radius: %2px;
                    padding: 4px 12px;
                    color: palette(button-text);
                }
                QPushButton:hover { background-color: %3; }
                QPushButton:pressed { background-color: %4; }
                QPushButton:disabled { color: %5; background-color: palette(window); border-color: %4; }
            )"
            )
            .arg(
                inputBorder(t).name(),
                QString::number(gc_inputRadius),
                menuHover(t).name(),
                menuBorder(t).name(),
                menuDisabled(t).name()
            );
    }

    /// @brief IntelliJ inspired menu bar and menus style
    inline QString menuStyle(const ThemeColors &t) {
        return QStringLiteral(
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
               .arg(menuHover(t).name(), menuBorder(t).name(), menuDisabled(t).name())
               .arg(gc_itemRadius);
    }

    // ==== app-specific QSS overrides (depend on cad widget names; stay in the app at phase 2) ====

    /// @brief IntelliJ inspired style for tool windows (widgets named "toolPanel")
    inline QString toolPanelStyle() {
        return QStringLiteral("#toolPanel { background-color: palette(base); border-radius: %1px; }")
            .arg(gc_cardRadius);
    }

    /// @brief Vim inspired status bar style
    inline QString statusBarStyle() {
        return QStringLiteral(
                R"(
                StatusBarWidget { background-color: palette(base); font-family: monospace; font-size: 12px; }
                StatusBarWidget QLabel { color: palette(text); }
                StatusBarWidget QLabel[modeActive="true"] { color: %1; font-weight: bold; }
            )"
            )
            .arg(gc_statusActive.name());
    }

    /// @brief App-colored strip the splitter draws between the viewport and tool window
    inline QString splitterStyle() {
        return QStringLiteral("QSplitter::handle { background-color: palette(window); }");
    }

    /// @brief Frameless window controls (objectName "windowButton"); the close button gets red accents
    inline QString windowButtonStyle(const ThemeColors &t) {
        return QStringLiteral(
                R"(
                QToolButton#windowButton { background: transparent; border: none; }
                QToolButton#windowButton:hover { background: %1; }
                QToolButton#windowButton:pressed { background: %2; }
                QToolButton#windowButton[close="true"]:hover { background: %3; }
                QToolButton#windowButton[close="true"]:pressed { background: %4; }
            )"
            )
            .arg(
                windowButtonOverlay(t, 0.08),
                windowButtonOverlay(t, 0.14),
                QStringLiteral("#E81123"),
                QStringLiteral("#C50E1F")
            );
    }

    /// @brief Full application stylesheet for a theme
    inline QString appStyleSheet(const ThemeColors &t) {
        return spinBoxStyle(t) + comboBoxStyle(t) + pushButtonStyle(t) + checkBoxStyle(t) + menuStyle(t) +
            toolPanelStyle()
            + statusBarStyle() + splitterStyle() + windowButtonStyle(t);
    }

    /// @brief Overlays the theme's colors onto the standard roles of a fresh standard palette
    inline QPalette buildPalette(QPalette palette, const ThemeColors &t) {
        palette.setColor(QPalette::Window, t.window);
        palette.setColor(QPalette::Base, t.base);
        palette.setColor(QPalette::AlternateBase, t.window);
        palette.setColor(QPalette::Button, t.window);
        palette.setColor(QPalette::ToolTipBase, t.base);
        palette.setColor(QPalette::Highlight, t.accent);
        palette.setColor(QPalette::HighlightedText, QColor(Qt::white));
        palette.setColor(QPalette::WindowText, t.text);
        palette.setColor(QPalette::Text, t.text);
        palette.setColor(QPalette::ButtonText, t.text);
        palette.setColor(QPalette::ToolTipText, t.text);
        palette.setColor(QPalette::PlaceholderText, menuDisabled(t));
        palette.setColor(QPalette::Disabled, QPalette::Text, menuDisabled(t));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, menuDisabled(t));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, menuDisabled(t));
        return palette;
    }

    /// @brief The theme currently applied; the GL viewport reads its clear color each frame
    inline const ThemeColors *g_active = &light;

    /// @brief Currently applied theme
    inline const ThemeColors& active() {
        return *g_active;
    }

    /// @brief Applies a theme to the running application (palette + stylesheet) live
    inline void apply(const ThemeColors &t) {
        g_active = &t;
        // start from a clean standard palette so switching back drops the other theme's overrides
        QApplication::setPalette(buildPalette(QApplication::style()->standardPalette(), t));
        qApp->setStyleSheet(appStyleSheet(t));
    }
}

#endif //CAD_THEME_HPP
