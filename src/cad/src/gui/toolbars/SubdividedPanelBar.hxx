//
// Created by Radosław Głasek on 07.07.2026
//

#ifndef CAD_SUBDIVIDEDPANELBAR_HXX
#define CAD_SUBDIVIDEDPANELBAR_HXX

#include <QWidget>

#include <map>
#include <optional>

class QSplitter;
class ToolPanelWidget;

/// @brief Fixed top/bottom dock, IntelliJ-style: at most 2
/// <tt>ToolPanelWidget</tt>s can be open at once (slot 0 = top, slot 1 =
/// bottom)
class SubdividedPanelBar final : public QWidget {
    Q_OBJECT

public:
    explicit SubdividedPanelBar(QWidget *parent = nullptr);

    /// @brief Register a panel under a stable bar index; not shown until
    /// <tt>showPanel</tt> docks it
    /// @returns bool indicating whether the register operation was successful
    bool registerPanel(int index, ToolPanelWidget *widget);

    /// @brief Dock the panel into slot (0 = top, 1 = bottom) and show the bar.
    /// Moves the panel if it's already open elsewhere, and displaces whatever
    /// previously occupied that slot (emitting <tt>panelClosedByUser</tt> for
    /// it)
    void showPanel(int index, int slot);

    /// @brief Undock a panel; hides its slot if that leaves it empty
    void hidePanel(int index);

    /// @brief Index of the panel that is an ancestor of the given widget
    /// (defaults to the application's current focus widget), or
    /// <tt>std::nullopt</tt>
    [[nodiscard]] std::optional<int> focusedPanelIndex(const QWidget *widget = nullptr) const;

signals:
    /// @brief A slot's own close button was clicked (or the panel was displaced
    /// by a dock), so the caller should uncheck the matching
    /// <tt>ToolPanelBar</tt> tab
    void panelClosedByUser(int index);

    /// @brief A drag-and-drop moved a panel into a different slot, so the tab
    /// bar can remember which slot to open it into next time
    void panelMovedToSlot(int index, int slot);

private:
    void dock(int panelIndex, int slot);

    class DockSlot;

    QSplitter *m_splitter;
    DockSlot *m_slots[2];
    std::map<int, ToolPanelWidget*> m_panels;
    std::map<int, int> m_panelSlot;
};

#endif //CAD_SUBDIVIDEDPANELBAR_HXX
