//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_CADTITLEBAR_HPP
#define CAD_CADTITLEBAR_HPP

#include <QToolButton>

#include "CadMenuBar.hpp"

/// @brief IntelliJ-style unified title bar for a frameless window
///
/// Hosts the application menu bar on the left and minimize / maximize / close controls on the right. 
/// The empty area drags the window; 
/// double-clicking it toggles the maximized state
class CadTitleBar final : public QWidget {
    Q_OBJECT

public:
    /// @brief Build a title bar embedding @p menuBar
    explicit CadTitleBar(QMenuBar *menuBar, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    bool event(QEvent *event) override;

private:
    QToolButton *m_minButton;
    QToolButton *m_maxButton;
    QToolButton *m_closeButton;

    void toggleMaximized() const;

    /// @brief Refresh the maximize/restore button icon for the window's current state
    void updateMaximizeButton() const;
};

/// @brief Install resize handling on a frameless top-level @p window
///
/// Watches the @p margin-wide border for hover/press and drives the platform's
/// native system-resize, so a FramelessWindowHint window stays resizable
void enableFramelessResize(QWidget *window, int margin = 6);

#endif //CAD_CADTITLEBAR_HPP
