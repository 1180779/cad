//
// Created by Radosław Głasek on 07.07.2026
//

#include "SubdividedPanelBar.hxx"

#include <functional>

#include <QSplitter>
#include <QToolButton>

#include "Theme.hpp"
#include "ToolPanelWidget.hpp"
#include "WidgetBuilders.hxx"

// ReSharper disable CppDFAMemoryLeak
/// @brief Fixed dock position: a small header (name + close button) over the
/// docked panel's content; hidden (0 height in the splitter) while empty
class SubdividedPanelBar::DockSlot final : public QWidget {
public:
    explicit DockSlot(QWidget *parent)
    : QWidget(parent) {
        m_header = new QWidget(this);
        const auto headerLayout = new QHBoxLayout(m_header);
        headerLayout->setContentsMargins(6, 2, 2, 2);
        m_title = widgets::addTitle(headerLayout, "");

        m_closeBtn = new QToolButton(this);
        m_closeBtn->setObjectName(QStringLiteral("panelCloseButton"));
        m_closeBtn->setCursor(Qt::ArrowCursor);
        m_closeBtn->setIconSize({9, 9});
        applyCloseIcon();
        const auto condOnCloseRequested = [this] {
            if (onCloseRequested) {
                onCloseRequested();
            }
        };
        connect(m_closeBtn, &QToolButton::clicked, this, condOnCloseRequested);

        headerLayout->addStretch();
        headerLayout->addWidget(m_closeBtn);

        m_contentLayout = new QVBoxLayout;
        m_contentLayout->setContentsMargins(0, 0, 0, 0);

        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_header);
        layout->addLayout(m_contentLayout, 1);
    }

    // ReSharper restore CppDFAMemoryLeak

    void setContent(ToolPanelWidget *widget) {
        if (m_content) {
            m_contentLayout->removeWidget(m_content);
            m_content->setParent(nullptr);
        }
        m_content = widget;
        if (m_content) {
            m_contentLayout->addWidget(m_content);
            m_title->setText(m_content->panelName());
        }
        else {
            m_title->clear();
        }
    }

    [[nodiscard]] ToolPanelWidget* content() const {
        return m_content;
    }

    [[nodiscard]] int dockedPanelIndex() const {
        return m_dockedPanelIndex;
    }

    void setDockedPanelIndex(const int index) {
        m_dockedPanelIndex = index;
    }

    std::function<void()> onCloseRequested;

protected:
    void changeEvent(QEvent *event) override {
        if (event->type() == QEvent::PaletteChange) {
            applyCloseIcon();
        }
        QWidget::changeEvent(event);
    }

private:
    QWidget *m_header;
    QLabel *m_title;
    QToolButton *m_closeBtn;
    QVBoxLayout *m_contentLayout;
    ToolPanelWidget *m_content = nullptr;
    int m_dockedPanelIndex = -1;

    void applyCloseIcon() const {
        m_closeBtn->setIcon(
            QIcon(theme::recoloredIcon(QStringLiteral("remove"), palette().color(QPalette::ButtonText)))
        );
    }
};

// ReSharper disable CppDFAMemoryLeak

SubdividedPanelBar::SubdividedPanelBar(QWidget *parent)
: QWidget(parent) {
    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(4);

    m_slots[0] = new DockSlot(m_splitter);
    m_slots[1] = new DockSlot(m_splitter);
    m_splitter->addWidget(m_slots[0]);
    m_splitter->addWidget(m_slots[1]);
    m_slots[0]->hide();
    m_slots[1]->hide();

    for (auto &slot : m_slots) {
        slot->onCloseRequested = [this, &slot] {
            const int panelIndex = slot->dockedPanelIndex();
            if (panelIndex < 0) {
                return;
            }
            hidePanel(panelIndex);
            emit panelClosedByUser(panelIndex);
        };
    }

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);

    hide();
}

// ReSharper restore CppDFAMemoryLeak

bool SubdividedPanelBar::registerPanel(const int index, ToolPanelWidget *widget) {
    const auto [iterator, wasInserted] = m_panels.emplace(index, widget);
    return wasInserted;
}

void SubdividedPanelBar::showPanel(const int index, const int slot) {
    if (!m_panels.contains(index)) {
        return;
    }
    show();
    dock(index, slot);
}

void SubdividedPanelBar::hidePanel(const int index) {
    const auto it = m_panelSlot.find(index);
    if (it == m_panelSlot.cend()) {
        return;
    }
    const int slot = it->second;
    m_panelSlot.erase(it);
    m_slots[slot]->setContent(nullptr);
    m_slots[slot]->setDockedPanelIndex(-1);
    m_slots[slot]->hide();
    if (m_slots[0]->isHidden() && m_slots[1]->isHidden()) {
        hide();
    }
}

std::optional<int> SubdividedPanelBar::focusedPanelIndex(const QWidget *widget) const {
    const QWidget *focusWidget = widget
                                     ? widget
                                     : QApplication::focusWidget();
    if (!focusWidget) {
        return std::nullopt;
    }
    for (const auto [fst, snd] : m_panelSlot) {
        if (m_slots[snd]->content() && m_slots[snd]->content()->isAncestorOf(focusWidget)) {
            return fst;
        }
    }
    return std::nullopt;
}

void SubdividedPanelBar::dock(const int panelIndex, const int slot) {
    const auto it = m_panels.find(panelIndex);
    ToolPanelWidget *widget = it != m_panels.cend()
                                  ? it->second
                                  : nullptr;
    if (!widget) {
        return;
    }

    const auto oldIt = m_panelSlot.find(panelIndex);
    const int oldSlot = oldIt != m_panelSlot.cend()
                            ? oldIt->second
                            : -1;
    if (oldSlot == slot) {
        widget->setFocus(Qt::OtherFocusReason);
        return;
    }
    if (oldSlot >= 0) {
        m_slots[oldSlot]->setContent(nullptr);
        m_slots[oldSlot]->setDockedPanelIndex(-1);
        m_slots[oldSlot]->hide();
        m_panelSlot.erase(panelIndex);
    }

    if (m_slots[slot]->content() != nullptr) {
        const int displaced = m_slots[slot]->dockedPanelIndex();
        m_panelSlot.erase(displaced);
        emit panelClosedByUser(displaced);
    }

    m_slots[slot]->setContent(widget);
    m_slots[slot]->setDockedPanelIndex(panelIndex);
    m_slots[slot]->show();
    m_panelSlot.emplace(panelIndex, slot);
    widget->setFocus(Qt::OtherFocusReason);
}
