//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_TOOLPANELBAR_HPP
#define CAD_TOOLPANELBAR_HPP

#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

/// @brief A rotated-text QToolButton used as a side-panel tab
class PanelTabButton final : public QToolButton {
    Q_OBJECT

public:
    explicit PanelTabButton(const QString &text, QWidget *parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

    [[nodiscard]] QSize minimumSizeHint() const override;

    /// @brief Whether the open panel currently holds focus; only a focused active tab
    /// paints with the accent color, otherwise it falls back to the hover color
    void setPanelFocused(bool focused);

protected:
    void paintEvent(QPaintEvent *event) override;

    /// @brief No-op so the button never auto-toggles; ToolPanelBar owns the checked state
    void nextCheckState() override;

private:
    /// @brief Cached focus state pushed in via setPanelFocused. paintEvent can't reach the
    /// panel stack or focus widget to compute this itself, so the fact is forwarded down 
    /// and stored here for the painter to read
    bool m_panelFocused = false;
};

/// @brief Thin vertical strip of checkable panel-tab buttons, IntelliJ-style
///
/// Emits panelRequested(index) when a button is activated, and
/// panelClosed() when the active button is unchecked (clicking the open panel's tab)
class ToolPanelBar final : public QWidget {
    Q_OBJECT

public:
    explicit ToolPanelBar(QWidget *parent = nullptr);

    /// @brief Add a panel entry; returns the button so callers can connect it to a QAction
    PanelTabButton* addPanel(const QString &name);

    /// @brief Return the button at index (nullptr if out of range)
    [[nodiscard]] PanelTabButton* buttonAt(int index) const;

    /// @brief Number of registered panels
    [[nodiscard]] int count() const;

    /// @brief Programmatically open a panel without emitting panelRequested
    void openPanel(int index);

    /// @brief Reflect whether the open tool panel holds keyboard focus, so the active
    /// tab shows the blue accent only while focused and the hover gray otherwise
    void setPanelFocused(bool focused) const;

signals:
    void panelRequested(int index);

    void panelClosed();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVBoxLayout *m_layout;
    QList<PanelTabButton*> m_buttons;
    int m_activeIndex = -1;

    /// @brief Set the active panel and derive all button checked states from it
    void setActiveIndex(int index);

    void onButtonClicked(int index);
};

#endif //CAD_TOOLPANELBAR_HPP
