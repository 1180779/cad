//
// Created by Radosław Głasek on 07.07.2026
//

#include "ToolPanelButton.hxx"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include "Theme.hpp"

PanelTabButton::PanelTabButton(const QString &text, QWidget *parent)
: QToolButton(parent) {
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

void PanelTabButton::nextCheckState() {}

void PanelTabButton::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStart = event->pos();
    }
    QToolButton::mousePressEvent(event);
}

void PanelTabButton::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton &&
        (event->pos() - m_dragStart).manhattanLength() >= QApplication::startDragDistance()) {
        emit dragRequested();
        return;
    }
    QToolButton::mouseMoveEvent(event);
}

void PanelTabButton::setPanelFocused(const bool focused) {
    if (m_panelFocused == focused) {
        return;
    }
    m_panelFocused = focused;
    update();
}

void PanelTabButton::paintEvent(QPaintEvent *) {
    // paint the tab explicitly since delegating to the style's renderer seems
    // to produce visual artifacts in some cases
    const QPalette &pal = palette();

    // intelliJ-style states:
    // distinct color when idle,
    // distinct color on hover,
    // distinct color when the open panel is focused;
    // an active-but-unfocused tab falls back to the hover style
    QColor bg;
    QColor fg = pal.color(QPalette::ButtonText);
    if (isChecked() && m_panelFocused) {
        bg = pal.color(QPalette::Highlight);
        fg = pal.color(QPalette::HighlightedText);
    }
    else if (isChecked() || underMouse()) {
        bg = pal.color(QPalette::Button);
    }
    else {
        bg = pal.color(QPalette::Base);
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF body = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    QPainterPath path;
    path.addRoundedRect(body, theme::gc_itemRadius, theme::gc_itemRadius);
    p.fillPath(path, bg);
    p.setPen(QPen(pal.color(QPalette::Mid), 1));
    p.drawPath(path);

    // rotated text
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
