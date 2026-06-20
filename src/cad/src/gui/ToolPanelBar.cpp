#include "ToolPanelBar.hpp"

#include <QStyleOptionToolButton>
#include <QStylePainter>

// ── PanelTabButton ────────────────────────────────────────────────────────────

PanelTabButton::PanelTabButton(const QString &text, QWidget *parent) : QToolButton(parent) {
    setText(text);
    setCheckable(true);
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize PanelTabButton::sizeHint() const {
    // swap width/height so the rotated text fits
    const QSize base = QToolButton::sizeHint();
    return {base.height(), base.width() + 8};
}

QSize PanelTabButton::minimumSizeHint() const {
    return sizeHint();
}

void PanelTabButton::paintEvent(QPaintEvent *) {
    QStylePainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    // rotate the painter 90° counter-clockwise around the widget center
    p.translate(0, height());
    p.rotate(-90);
    opt.rect = QRect(0, 0, width(), height());
    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

// ── ToolPanelBar ─────────────────────────────────────────────────────────────

ToolPanelBar::ToolPanelBar(QWidget *parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(2, 4, 2, 4);
    m_layout->setSpacing(2);
    m_layout->addStretch();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

PanelTabButton* ToolPanelBar::addPanel(const QString &name) {
    const int index = m_buttons.size();
    auto *btn = new PanelTabButton(name, this);

    // insert before the trailing stretch
    m_layout->insertWidget(index, btn);
    m_buttons.append(btn);

    connect(
        btn,
        &QToolButton::toggled,
        this,
        [this, index](const bool checked) {
            onButtonToggled(index, checked);
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

void ToolPanelBar::onButtonToggled(const int index, const bool checked) {
    if (checked) {
        // uncheck all others (exclusive)
        for (int i = 0; i < m_buttons.size(); ++i) {
            if (i != index) {
                QSignalBlocker blocker(m_buttons[i]);
                m_buttons[i]->setChecked(false);
            }
        }
        m_activeIndex = index;
        emit panelRequested(index);
    }
    else {
        if (m_activeIndex == index) {
            m_activeIndex = -1;
            emit panelClosed();
        }
    }
}
