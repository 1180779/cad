//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_THEME_HPP
#define CAD_THEME_HPP

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>
#include <QStyle>

/// @brief Centralized custom theme palette
namespace theme {
    /// @brief A single theme's seed colors
    struct ThemeColors {
        /// @brief App background (QPalette::Window)
        QColor window;

        /// @brief  Tool-window card / input base (QPalette::Base)
        QColor base;

        /// @brief Selection / highlight (QPalette::Highlight)
        QColor accent;

        /// @brief Primary foreground
        QColor text;

        /// @brief OpenGL clear color for the non-stereo viewport
        QColor viewport;

        // viewport geometry colors

        /// @brief Basic geometry lines
        QColor line;

        /// @brief Unselected control points
        QColor point;

        /// @brief Tessellated Bézier curves
        QColor curve;

        /// @brief Infinite grid minor lines
        QColor gridMinor;

        /// @brief Infinite grid major lines
        QColor gridMajor;

        /// @brief Flips derived tints between darker()/lighter()
        bool dark;
    };

    /// @brief Soft off-white background, white cards, dark text
    inline constexpr ThemeColors gc_light{
        .window = QColor(0xF2, 0xF3, 0xF5),
        .base = QColor(0xFF, 0xFF, 0xFF),
        .accent = QColor(0x56, 0xA0, 0xEB),
        .text = QColor(0x1A, 0x1A, 0x1A),
        .viewport = QColor(0xFF, 0xFF, 0xFF),
        .line = QColor(0x00, 0x00, 0x00),
        .point = QColor(0x14, 0x14, 0x14),
        .curve = QColor(0x80, 0x80, 0x80),
        .gridMinor = QColor(0xB8, 0xB8, 0xB8),
        .gridMajor = QColor(0x80, 0x80, 0x80),
        .dark = false
    };

    /// @brief Dark background, slightly darker cards, light text
    inline constexpr ThemeColors gc_dark{
        .window = QColor(0x2B, 0x2D, 0x30),
        .base = QColor(0x1E, 0x1F, 0x22),
        .accent = QColor(0x56, 0xA0, 0xEB),
        .text = QColor(0xDF, 0xE1, 0xE5),
        .viewport = QColor(0x1E, 0x1F, 0x22),
        .line = QColor(0xD7, 0xD9, 0xDD),
        .point = QColor(0xE0, 0xE2, 0xE6),
        .curve = QColor(0xA8, 0xAB, 0xB0),
        .gridMinor = QColor(0x4A, 0x4D, 0x52),
        .gridMajor = QColor(0x6A, 0x6E, 0x74),
        .dark = true
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

    // derived tints (theme-relative: darken light bg, lighten dark bg)

    /// @brief Shifts the window color toward contrast by @p amount (Qt darker/lighter scale)
    inline QColor tint(const ThemeColors &t, const int amount) {
        return t.dark
                   ? t.window.lighter(amount)
                   : t.window.darker(amount);
    }

    /// @brief Subtle hover wash
    inline QColor menuHover(const ThemeColors &t) {
        return tint(t, 112);
    }

    /// @brief Muted text
    inline QColor menuDisabled(const ThemeColors &t) {
        return tint(
            t,
            t.dark
                ? 185
                : 140
        );
    }

    /// @brief Dropdown border / separators
    inline QColor menuBorder(const ThemeColors &t) {
        return tint(t, 125);
    }

    /// @brief Resting input border
    inline QColor inputBorder(const ThemeColors &t) {
        return tint(t, 125);
    }

    /// @brief Translucent overlay for frameless window-control hover
    inline QString windowButtonOverlay(const ThemeColors &t, const double a) {
        return t.dark
                   ? QStringLiteral("rgba(255, 255, 255, %1)").arg(a)
                   : QStringLiteral("rgba(0, 0, 0, %1)").arg(a);
    }

    /// @brief Recolors a `currentColor` SVG resource to @p color and returns a file path for QSS url()
    /// @details The SVGs use fill="currentColor" which QSvgRenderer resolves to black, so they vanish
    /// on dark backgrounds. We substitute the literal color and cache one file per (name, color) in the
    /// per-user OS cache dir (<code>QStandardPaths::CacheLocation</code>), regenerating only on a miss
    /// @param name resource basename under :/icons/ without extension
    /// @param color target color to replace "currentColor" with in the SVG
    inline QString recoloredIcon(const QString &name, const QColor &color) {
        static const QString cacheDir = [] {
            const QString d = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                + QStringLiteral("/icons");
            return QDir().mkpath(d)
                       ? d
                       : QString();
        }();
        const QString src = QStringLiteral(":/icons/%1.svg").arg(name);
        if (cacheDir.isEmpty()) {
            return src;
        }
        const QString out = QStringLiteral("%1/%2-%3.svg").arg(cacheDir, name, color.name().mid(1));
        if (QFileInfo::exists(out)) {
            return out;
        }
        QFile in(src);
        if (!in.open(QIODevice::ReadOnly)) {
            return src; // resource missing
        }
        QByteArray svg = in.readAll();
        svg.replace("currentColor", color.name().toUtf8());
        if (QSaveFile f(out);
            f.open(QIODevice::WriteOnly) && f.write(svg) == svg.size() && f.commit()) {
            return out;
        }
        return src;
    }

    /// @brief IntelliJ inspired spin box QSS
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

    /// @brief IntelliJ inspired combo box QSS
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

    /// @brief IntelliJ inspired check box QSS
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

    /// @brief IntelliJ inspired push button QSS
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

    /// @brief IntelliJ inspired menu bar and menus QSS
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

    /// @brief IntelliJ inspired tool windows QSS (widgets named "toolPanel")
    inline QString toolPanelStyle() {
        return QStringLiteral("#toolPanel { background-color: palette(base); border-radius: %1px; }")
            .arg(gc_cardRadius);
    }

    /// @brief Vim inspired status bar QSS
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

    /// @brief App-colored strip the splitter draws between the viewport and tool window QSS
    inline QString splitterStyle() {
        return QStringLiteral("QSplitter::handle { background-color: palette(window); }");
    }

    /// @brief Frameless window controls QSS (objectName "windowButton"); 
    /// the close button gets red accents
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

    /// @brief Full stylesheet for a theme
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

    /// @brief Currently applied theme
    inline const ThemeColors *g_active = &gc_light;

    /// @brief Currently applied theme
    inline const ThemeColors& active() {
        return *g_active;
    }

    /// @brief Applies a theme to the running application (palette + stylesheet) live
    inline void apply(const ThemeColors &t) {
        g_active = &t;
        QApplication::setPalette(buildPalette(QApplication::style()->standardPalette(), t));
        qApp->setStyleSheet(appStyleSheet(t));
    }
}

#endif //CAD_THEME_HPP
