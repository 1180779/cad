//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_TOOLPANELBAR_HPP
#define CAD_TOOLPANELBAR_HPP

#include "ToolPanelButton.hxx"

/// @brief MIME type used to drag a panel index out of <tt>ToolPanelBar</tt>
/// into a <tt>SubdividedPanelBar</tt>
inline const auto g_panelIndexMimeType = QStringLiteral("application/x-cad-panel-index");

class ToolPanelBar;

namespace aliases {
    // ReSharper disable once CppInconsistentNaming
    using ToolPB = ToolPanelBar;
}

/// @brief Thin vertical strip of checkable panel-tab buttons. Two tabs can be
/// checked (open) independently at once
/// 
/// @note Buttons never auto-toggle (see
/// <tt>PanelTabButton::nextCheckState</tt>). The bar owns and flips the checked
/// state explicitly
class ToolPanelBar final : public QWidget {
    Q_OBJECT

public:
    explicit ToolPanelBar(QWidget *parent = nullptr);

    /// @brief Add a panel entry 
    /// @returns Associated panel button so callers can connect it to a
    /// <tt>QAction</tt>
    PanelTabButton* addPanel(const QString &name, bool topLayout = true);

    /// @brief Return the button at index (<tt>nullptr</tt> if out of range)
    [[nodiscard]] PanelTabButton* buttonAt(int index) const;

    /// @brief Number of registered panels
    [[nodiscard]] int count() const;

    /// @brief Mark a panel's tab open (checked) without emitting
    /// <tt>panelRequested</tt>
    void openPanel(int index);

    /// @brief Reflect whether panel index's tool panel holds keyboard focus, so
    /// its tab shows the accent color only while focused and the hover color
    /// otherwise. Several tabs can be open (checked) at once, but at most one
    /// is focused
    void setPanelFocused(int index, bool focused) const;

    /// @brief Which dock slot (0 = top, 1 = bottom) clicking this tab opens
    /// into
    [[nodiscard]] int groupOf(int index) const;

    /// @brief Move a tab into the top (0) or bottom (1) group and re-layout
    void setGroup(int index, int group);

    /// @brief Watches the tab buttons for external show/hide (Tools menu) to
    /// keep the divider visibility in sync
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    /// @brief A tab was clicked to open its panel
    void panelRequested(int index);

    /// @brief A tab was clicked to close its (already open) panel
    void panelClosed(int index);

protected:
    void paintEvent(QPaintEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void dropEvent(QDropEvent *event) override;

private:
    void onButtonClicked(int index);

    /// @brief Show the divider only while both groups have visible content
    /// (tabs or the drag placeholder)
    void updateDivider() const;

    /// @brief Dashed accent-outlined block used as drop marker during drags; @p
    /// filled adds the translucent accent wash
    [[nodiscard]] QWidget* makeDropMarker(const QSize &size, bool filled);

    /// @brief Build the MIME payload and run the QDrag for panel @p index;
    /// invoked via PanelTabButton::dragRequested
    void startDrag(int index) const;

    /// @brief Layout position in @p layout that a drop at bar-relative @p y
    /// maps to (first visible tab whose midpoint lies below y)
    [[nodiscard]] int dropIndexIn(const QVBoxLayout *layout, int y) const;

    /// @brief Hide panelIndex's real tab and show the placeholder at the
    /// position @p y implies, so the bar visibly previews the drop before it
    /// happens
    void showDragPreview(int panelIndex, int y);

    /// @brief Remove the placeholder, restore the dragged tab, and re-layout
    void clearDragPreview();

    QVBoxLayout *m_topLayout;
    QVBoxLayout *m_bottomLayout;
    QWidget *m_divider;

    /// @brief Stable by panel index, mirrors registration order; never
    /// reordered. Visual order lives in <tt>m_topLayout</tt> /
    /// <tt>m_bottomLayout</tt> directly
    QList<PanelTabButton*> m_buttons;

    /// @brief Panel index currently being dragged (its own button is hidden and
    /// a placeholder block stands in for it), or -1 when no drag is in progress
    int m_draggingIndex = -1;

    /// @brief Dashed accent marker showing where a drag would land; created on
    /// first dragMoveEvent, destroyed once the drag ends
    QWidget *m_dragPlaceholder = nullptr;

    /// @brief Same marker shown in a group that has no visible tabs during a
    /// drag, so an empty group stays a visible drop target (IntelliJ-style)
    QWidget *m_emptyGroupHint = nullptr;
};

#endif //CAD_TOOLPANELBAR_HPP
