//
// Created by Radosław Głasek on 21.06.2026
//

#include "ToolPanelBar.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>

// ── PanelTabButton ────────────────────────────────────────────────────────────

PanelTabButton::PanelTabButton(const QString &text, QWidget *parent) : QToolButton(parent) {
    setText(text);
    setCheckable(true);
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_Hover, true); // repaint on enter/leave so hover tint shows
}

QSize PanelTabButton::sizeHint() const {
    // swap width/height so the rotated text fits
    const QSize base = QToolButton::sizeHint();
    return {base.height(), base.width() + 8};
}

QSize PanelTabButton::minimumSizeHint() const {
    return sizeHint();
}

void PanelTabButton::nextCheckState() {
    // intentionally empty: ToolPanelBar drives the checked state explicitly,
    // so the button must not toggle itself on click
}

void PanelTabButton::paintEvent(QPaintEvent *) {
    // paint the tab explicitly rather than delegating to the style's complex-control
    // renderer: the latter swaps in highlight backgrounds / white text on transient
    // focus/sunken/"on" states, which produced intermittent style flips. Here the
    // appearance is a controlled function of just isChecked()/underMouse()
    const QPalette &pal = palette();

    // intelliJ-style states: white when idle, gray on hover, blue + white text when selected
    QColor bg;
    QColor fg = pal.color(QPalette::ButtonText);
    if (isChecked()) {
        bg = pal.color(QPalette::Highlight);
        fg = pal.color(QPalette::HighlightedText);
    }
    else if (underMouse()) {
        bg = pal.color(QPalette::Button);
    }
    else {
        bg = pal.color(QPalette::Base);
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // rounded body with a subtle border
    const QRectF body = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    QPainterPath path;
    path.addRoundedRect(body, 3, 3);
    p.fillPath(path, bg);
    p.setPen(QPen(pal.color(QPalette::Mid), 1));
    p.drawPath(path);

    // rotated text, baseline-centered within the body
    p.save();
    p.translate(rect().center());
    p.rotate(-90);
    p.setPen(fg);
    const auto trLeft = -height() / 2;
    const auto trTop = -width() / 2;
    const auto trWidth = height();
    const auto trHeight = width();
    const QRect textRect(trLeft, trTop, trWidth, trHeight);
    p.drawText(textRect, Qt::AlignCenter, text());
    p.restore();
}

// ── ToolPanelBar ─────────────────────────────────────────────────────────────

ToolPanelBar::ToolPanelBar(QWidget *parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(3, 4, 3, 4);
    m_layout->setSpacing(2);
    m_layout->addStretch();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void ToolPanelBar::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter p(this);
    p.setPen(palette().color(QPalette::Mid));
    p.drawLine(0, 0, 0, height());
}

PanelTabButton* ToolPanelBar::addPanel(const QString &name) {
    const int index = m_buttons.size();
    auto *btn = new PanelTabButton(name, this);

    // insert before the trailing stretch
    m_layout->insertWidget(index, btn);
    m_buttons.append(btn);

    connect(
        btn,
        &QToolButton::clicked,
        this,
        [this, index] {
            onButtonClicked(index);
        }
    );

    return btn;
}

PanelTabButton* ToolPanelBar::buttonAt(const int index) const {
    if (index < 0 || index >= m_buttons.size()) {
        return nullptr;
    }
    return m_buttons[index];
}

int ToolPanelBar::count() const {
    return m_buttons.size();
}

void ToolPanelBar::openPanel(const int index) {
    if (index < 0 || index >= m_buttons.size()) {
        return;
    }
    setActiveIndex(index);
}

void ToolPanelBar::setActiveIndex(const int index) {
    // single source of truth: button checked states are derived from m_activeIndex,
    // never from the buttons' own (now disabled) auto-toggle
    m_activeIndex = index;
    for (int i = 0; i < m_buttons.size(); ++i) {
        m_buttons[i]->setChecked(i == index);
    }
}

void ToolPanelBar::onButtonClicked(const int index) {
    if (m_activeIndex == index) {
        // clicking the open panel's tab closes it
        setActiveIndex(-1);
        emit panelClosed();
    }
    else {
        setActiveIndex(index);
        emit panelRequested(index);
    }
}
