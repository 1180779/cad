#include "CadTitleBar.hpp"

#include <QEvent>
#include <QHBoxLayout>
#include <QHoverEvent>
#include <QMenuBar>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>
#include <QWindow>

namespace {
    constexpr int kTitleBarHeight = 30;

    QToolButton* makeWindowButton(QWidget *parent, const QStyle::StandardPixmap icon) {
        auto *btn = new QToolButton(parent);
        btn->setIcon(parent->style()->standardIcon(icon));
        btn->setAutoRaise(true);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setFixedSize(kTitleBarHeight + 12, kTitleBarHeight);
        return btn;
    }

    /// @brief Drives native system-resize from the border of a frameless window.
    /// No Q_OBJECT/signals, so it needs no moc pass.
    class FramelessResizeFilter final : public QObject {
    public:
        FramelessResizeFilter(QWidget *window, const int margin) : QObject(window), m_window(window), m_margin(margin) {
        }

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override {
            if (watched != m_window) {
                return QObject::eventFilter(watched, event);
            }

            switch (event->type()) {
            case QEvent::MouseMove: {
                if (!m_window->isMaximized()) {
                    const auto *me = static_cast<QMouseEvent*>(event);
                    if (const Qt::Edges edges = edgesAt(me->pos());
                        edges != Qt::Edges()) {
                        m_window->setCursor(cursorForEdges(edges));
                    }
                    else {
                        m_window->unsetCursor();
                    }
                }
                break;
            }
            case QEvent::Leave:
                // pointer entered a child widget: drop any resize cursor so it
                // doesn't stick on the interior content
                m_window->unsetCursor();
                break;
            case QEvent::MouseButtonPress: {
                const auto *me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton && !m_window->isMaximized()) {
                    if (const Qt::Edges edges = edgesAt(me->pos());
                        edges != Qt::Edges()) {
                        if (QWindow *handle = m_window->windowHandle()) {
                            handle->startSystemResize(edges);
                            return true;
                        }
                    }
                }
                break;
            }
            default:
                break;
            }
            return QObject::eventFilter(watched, event);
        }

    private:
        QWidget *m_window;
        int m_margin;

        [[nodiscard]] Qt::Edges edgesAt(const QPoint &pos) const {
            Qt::Edges edges;
            if (pos.x() <= m_margin)
                edges |= Qt::LeftEdge;
            if (pos.x() >= m_window->width() - m_margin)
                edges |= Qt::RightEdge;
            if (pos.y() <= m_margin)
                edges |= Qt::TopEdge;
            if (pos.y() >= m_window->height() - m_margin)
                edges |= Qt::BottomEdge;
            return edges;
        }

        [[nodiscard]] static Qt::CursorShape cursorForEdges(const Qt::Edges edges) {
            if ((edges & Qt::LeftEdge && edges & Qt::TopEdge) ||
                (edges & Qt::RightEdge && edges & Qt::BottomEdge)) {
                return Qt::SizeFDiagCursor;
            }
            if ((edges & Qt::RightEdge && edges & Qt::TopEdge) ||
                (edges & Qt::LeftEdge && edges & Qt::BottomEdge)) {
                return Qt::SizeBDiagCursor;
            }
            if (edges & (Qt::LeftEdge | Qt::RightEdge))
                return Qt::SizeHorCursor;
            if (edges & (Qt::TopEdge | Qt::BottomEdge))
                return Qt::SizeVerCursor;
            return Qt::ArrowCursor;
        }
    };
}

CadTitleBar::CadTitleBar(QMenuBar *menuBar, QWidget *parent) : QWidget(parent) {
    setFixedHeight(kTitleBarHeight);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    menuBar->setParent(this);
    layout->addWidget(menuBar);
    layout->addStretch(); // draggable area

    m_minButton = makeWindowButton(this, QStyle::SP_TitleBarMinButton);
    m_maxButton = makeWindowButton(this, QStyle::SP_TitleBarMaxButton);
    m_closeButton = makeWindowButton(this, QStyle::SP_TitleBarCloseButton);
    layout->addWidget(m_minButton);
    layout->addWidget(m_maxButton);
    layout->addWidget(m_closeButton);

    connect(
        m_minButton,
        &QToolButton::clicked,
        this,
        [this] {
            window()->showMinimized();
        }
    );
    connect(
        m_maxButton,
        &QToolButton::clicked,
        this,
        [this] {
            toggleMaximized();
        }
    );
    connect(
        m_closeButton,
        &QToolButton::clicked,
        this,
        [this] {
            window()->close();
        }
    );
}

void CadTitleBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (QWindow *handle = window()->windowHandle()) {
            handle->startSystemMove();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void CadTitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        toggleMaximized();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool CadTitleBar::event(QEvent *event) {
    // keep the maximize/restore glyph in sync with the window state
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeButton();
    }
    return QWidget::event(event);
}

void CadTitleBar::toggleMaximized() {
    QWidget *w = window();
    if (w->isMaximized()) {
        w->showNormal();
    }
    else {
        w->showMaximized();
    }
    updateMaximizeButton();
}

void CadTitleBar::updateMaximizeButton() {
    const auto icon = window()->isMaximized()
                          ? QStyle::SP_TitleBarNormalButton
                          : QStyle::SP_TitleBarMaxButton;
    m_maxButton->setIcon(style()->standardIcon(icon));
}

void enableFramelessResize(QWidget *window, const int margin) {
    window->setMouseTracking(true);
    window->installEventFilter(new FramelessResizeFilter(window, margin));
}
