#pragma once

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

protected:
    void paintEvent(QPaintEvent *event) override;
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

signals:
    void panelRequested(int index);

    void panelClosed();

private:
    QVBoxLayout *m_layout;
    QList<PanelTabButton*> m_buttons;
    int m_activeIndex = -1;

    void onButtonToggled(int index, bool checked);

    void paintEvent(QPaintEvent *event) override;
};
