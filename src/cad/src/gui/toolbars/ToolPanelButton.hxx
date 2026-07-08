//
// Created by Radosław Głasek on 07.07.2026
//

#ifndef CAD_TOOLPANELBUTTON_HXX
#define CAD_TOOLPANELBUTTON_HXX

#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

/// @brief A rotated-text <tt>QToolButton</tt> used as a side-panel tab
class PanelTabButton final : public QToolButton {
    Q_OBJECT

public:
    explicit PanelTabButton(const QString &text, QWidget *parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

    [[nodiscard]] QSize minimumSizeHint() const override;

    /// @brief Whether the open panel currently holds focus; only a focused
    /// active tab paints with the accent color, otherwise it falls back to the
    /// hover color
    void setPanelFocused(bool focused);

signals:
    /// @brief Emitted once the mouse moved past the drag threshold with the
    /// left button held
    void dragRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

    /// @brief No-op so the button never auto-toggles
    void nextCheckState() override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

private:
    /// @brief Cached focus state pushed in via <tt>setPanelFocused</tt>.
    /// <tt>paintEvent</tt> can't reach the panel stack or focus widget to
    /// compute this itself, so the fact is forwarded down and stored here for
    /// the painter to read
    bool m_panelFocused = false;
    QPoint m_dragStart;
};

#endif //CAD_TOOLPANELBUTTON_HXX
