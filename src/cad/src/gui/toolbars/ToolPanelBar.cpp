//
// Created by Radosław Głasek on 21.06.2026
//

#include "ToolPanelBar.hpp"

#include <QCursor>
#include <QDrag>
#include <QDropEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>

#include "../Theme.hpp"
#include "gui/WidgetBuilders.hxx"

ToolPanelBar::ToolPanelBar(QWidget *parent)
: QWidget(parent) {
    // ReSharper disable once CppDFAMemoryLeak
    const auto outer = new QVBoxLayout(this);
    outer->setContentsMargins(3, 4, 3, 4);
    outer->setSpacing(2);

    // ReSharper disable once CppDFAMemoryLeak
    const auto topContainer = new QWidget(this);
    m_topLayout = new QVBoxLayout(topContainer);
    m_topLayout->setContentsMargins(0, 0, 0, 0);
    m_topLayout->setSpacing(2);

    m_divider = widgets::horizontalLine(this);

    // ReSharper disable once CppDFAMemoryLeak
    const auto bottomContainer = new QWidget(this);
    m_bottomLayout = new QVBoxLayout(bottomContainer);
    m_bottomLayout->setContentsMargins(0, 0, 0, 0);
    m_bottomLayout->setSpacing(2);

    outer->addWidget(topContainer);
    outer->addWidget(m_divider);
    outer->addWidget(bottomContainer);
    outer->addStretch(1);
    m_divider->hide();

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setAcceptDrops(true);
}

void ToolPanelBar::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter p(this);
    p.setPen(palette().color(QPalette::Mid));
    p.drawLine(0, 0, 0, height());
}

PanelTabButton* ToolPanelBar::addPanel(const QString &name, const bool topLayout) {
    const int index = static_cast<int>(m_buttons.size());
    auto *btn = new PanelTabButton(name, this);

    m_buttons.append(btn);
    const auto layout = topLayout
                            ? m_topLayout
                            : m_bottomLayout;
    layout->addWidget(btn);
    btn->installEventFilter(this);
    updateDivider();
    const auto onIndexButtonClicked = [this, index] {
        onButtonClicked(index);
    };
    const auto onIndexStartDrag = [this, index] {
        startDrag(index);
    };

    connect(btn, &QToolButton::clicked, this, onIndexButtonClicked);
    connect(btn, &PanelTabButton::dragRequested, this, onIndexStartDrag);
    return btn;
}

void ToolPanelBar::startDrag(const int index) const {
    PanelTabButton *btn = buttonAt(index);
    if (!btn) {
        return;
    }
    // ReSharper disable once CppDFAMemoryLeak
    const auto mime = new QMimeData;
    mime->setData(g_panelIndexMimeType, QByteArray::number(index));

    const QPixmap pixmap = btn->grab();
    // ReSharper disable once CppDFAMemoryLeak
    const auto opacity = new QGraphicsOpacityEffect(btn);
    opacity->setOpacity(0.35);
    btn->setGraphicsEffect(opacity);

    // ReSharper disable once CppDFAMemoryLeak
    const auto drag = new QDrag(btn);
    drag->setMimeData(mime);
    drag->setPixmap(pixmap);
    drag->setHotSpot(btn->mapFromGlobal(QCursor::pos()));
    drag->exec(Qt::MoveAction);

    btn->setGraphicsEffect(nullptr);
}

PanelTabButton* ToolPanelBar::buttonAt(const int index) const {
    if (index < 0 || index >= m_buttons.size()) {
        return nullptr;
    }
    return m_buttons[index];
}

int ToolPanelBar::count() const {
    return static_cast<int>(m_buttons.size());
}

void ToolPanelBar::openPanel(const int index) {
    if (auto *btn = buttonAt(index)) {
        btn->setChecked(true);
        emit panelRequested(index);
    }
}

void ToolPanelBar::setPanelFocused(const int index, const bool focused) const {
    if (auto *btn = buttonAt(index)) {
        btn->setPanelFocused(focused);
    }
}

int ToolPanelBar::groupOf(const int index) const {
    return index >= 0 && index < m_buttons.size() &&
           m_bottomLayout->indexOf(m_buttons[index]) != -1
               ? 1
               : 0;
}

void ToolPanelBar::setGroup(const int index, const int group) {
    if (index < 0 || index >= m_buttons.size() || groupOf(index) == group) {
        return;
    }
    (group == 0
         ? m_topLayout
         : m_bottomLayout)->addWidget(m_buttons[index]);
    updateDivider();
}

void ToolPanelBar::updateDivider() const {
    const auto hasVisibleItems = [](const QVBoxLayout *layout) {
        for (int i = 0; i < layout->count(); ++i) {
            if (const QWidget *w = layout->itemAt(i)->widget();
                w && !w->isHidden()) {
                return true;
            }
        }
        return false;
    };
    m_divider->setVisible(hasVisibleItems(m_topLayout) && hasVisibleItems(m_bottomLayout));
}

bool ToolPanelBar::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::ShowToParent || event->type() == QEvent::HideToParent) {
        updateDivider();
    }
    return QWidget::eventFilter(watched, event);
}

void ToolPanelBar::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat(g_panelIndexMimeType)) {
        event->acceptProposedAction();
    }
}

void ToolPanelBar::dragMoveEvent(QDragMoveEvent *event) {
    if (!event->mimeData()->hasFormat(g_panelIndexMimeType)) {
        return;
    }
    const int panelIndex = event->mimeData()->data(g_panelIndexMimeType).toInt();
    showDragPreview(panelIndex, event->position().toPoint().y());
    event->acceptProposedAction();
}

void ToolPanelBar::dragLeaveEvent(QDragLeaveEvent *) {
    clearDragPreview();
}

void ToolPanelBar::dropEvent(QDropEvent *event) {
    const int panelIndex = event->mimeData()->data(g_panelIndexMimeType).toInt();
    if (PanelTabButton *btn = buttonAt(panelIndex);
        btn && m_dragPlaceholder) {
        QVBoxLayout *target = m_topLayout->indexOf(m_dragPlaceholder) != -1
                                  ? m_topLayout
                                  : m_bottomLayout;
        m_topLayout->removeWidget(btn);
        m_bottomLayout->removeWidget(btn);
        target->insertWidget(target->indexOf(m_dragPlaceholder), btn);
    }
    clearDragPreview();
    event->acceptProposedAction();
}

int ToolPanelBar::dropIndexIn(const QVBoxLayout *layout, const int y) const {
    for (int i = 0; i < layout->count(); ++i) {
        const QWidget *w = layout->itemAt(i)->widget();
        if (!w || w == m_dragPlaceholder || w == m_emptyGroupHint || w->isHidden()) {
            continue;
        }
        if (w->mapTo(this, QPoint(0, w->height() / 2)).y() > y) {
            return i;
        }
    }
    return layout->count();
}

void ToolPanelBar::showDragPreview(const int panelIndex, const int y) {
    PanelTabButton *btn = buttonAt(panelIndex);
    if (!btn) {
        return;
    }
    if (m_draggingIndex != panelIndex) {
        clearDragPreview();
        m_draggingIndex = panelIndex;
        btn->hide();

        m_dragPlaceholder = makeDropMarker(btn->size(), true);
    }

    QVBoxLayout *target = y < m_topLayout->parentWidget()->geometry().bottom()
                              ? m_topLayout
                              : m_bottomLayout;
    m_topLayout->removeWidget(m_dragPlaceholder);
    m_bottomLayout->removeWidget(m_dragPlaceholder);
    target->insertWidget(dropIndexIn(target, y), m_dragPlaceholder);
    m_dragPlaceholder->show();

    QVBoxLayout *other = target == m_topLayout
                             ? m_bottomLayout
                             : m_topLayout;
    const auto hasVisibleTab = [this](const QVBoxLayout *layout) {
        for (int i = 0; i < layout->count(); ++i) {
            if (const QWidget *w = layout->itemAt(i)->widget();
                w && w != m_dragPlaceholder && w != m_emptyGroupHint && !w->isHidden()) {
                return true;
            }
        }
        return false;
    };
    if (!m_emptyGroupHint) {
        m_emptyGroupHint = makeDropMarker(btn->size(), false);
    }
    m_topLayout->removeWidget(m_emptyGroupHint);
    m_bottomLayout->removeWidget(m_emptyGroupHint);
    if (hasVisibleTab(other)) {
        m_emptyGroupHint->hide();
    }
    else {
        other->addWidget(m_emptyGroupHint);
        m_emptyGroupHint->show();
    }
    updateDivider();
}

QWidget* ToolPanelBar::makeDropMarker(const QSize &size, const bool filled) {
    // IntelliJ-style drop markers: the cursor placeholder is a solid accent
    // border over a translucent wash, the empty-group hint a hollow dashed
    // outline
    auto *marker = new QWidget(this);
    marker->setFixedSize(size);
    marker->setAttribute(Qt::WA_StyledBackground);
    const QColor accent = palette().color(QPalette::Highlight);
    QString style = QStringLiteral("border: 1px %1 %2; border-radius: %3px;")
                    .arg(
                        filled
                            ? QStringLiteral("solid")
                            : QStringLiteral("dashed"),
                        accent.name()
                    )
                    .arg(theme::gc_itemRadius);
    if (filled) {
        style += QStringLiteral("background-color: rgba(%1, %2, %3, 0.15);")
                 .arg(accent.red())
                 .arg(accent.green())
                 .arg(accent.blue());
    }
    marker->setStyleSheet(style);
    return marker;
}

void ToolPanelBar::clearDragPreview() {
    if (auto *btn = buttonAt(m_draggingIndex)) {
        btn->show();
    }
    m_draggingIndex = -1;
    if (m_dragPlaceholder) {
        m_dragPlaceholder->hide();
        m_dragPlaceholder->deleteLater();
        m_dragPlaceholder = nullptr;
    }
    if (m_emptyGroupHint) {
        m_emptyGroupHint->hide();
        m_emptyGroupHint->deleteLater();
        m_emptyGroupHint = nullptr;
    }
    updateDivider();
}

void ToolPanelBar::onButtonClicked(const int index) {
    PanelTabButton *btn = m_buttons[index];
    const bool nowOpen = !btn->isChecked();
    btn->setChecked(nowOpen);
    if (nowOpen) {
        emit panelRequested(index);
    }
    else {
        btn->setPanelFocused(false);
        emit panelClosed(index);
    }
}
