#include "StatusBarWidget.hpp"

#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOption>

static QLabel* makeSeparator(QWidget *parent) {
    const auto sep = new QLabel("|", parent);
    sep->setStyleSheet(StatusBarWidget::s_separatorStyle);
    sep->setContentsMargins(4, 0, 4, 0);
    return sep;
}

StatusBarWidget::StatusBarWidget(QWidget *parent) : QWidget(parent) {
    setFixedHeight(s_barHeight);
    setStyleSheet(s_barStyle);
    setAutoFillBackground(true);

    const auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_modeLabel = new QLabel("Mode: --", this);
    m_cameraLabel = new QLabel("Camera: --", this);
    m_newPointsTargetLabel = new QLabel("Bezier: --", this);
    m_selectionLabel = new QLabel("", this);

    for (QLabel *label : {m_modeLabel, m_cameraLabel, m_newPointsTargetLabel, m_selectionLabel}) {
        label->setContentsMargins(8, 0, 8, 0);
    }

    layout->addWidget(m_modeLabel);
    layout->addWidget(makeSeparator(this));
    layout->addWidget(m_cameraLabel);
    layout->addWidget(makeSeparator(this));
    layout->addWidget(m_newPointsTargetLabel);
    layout->addWidget(makeSeparator(this));
    layout->addWidget(m_selectionLabel);
    layout->addStretch();
}

void StatusBarWidget::paintEvent(QPaintEvent *) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void StatusBarWidget::setTransformMode(const TransformMode mode, const QString &axisInfo) const {
    refreshModeLabel(mode, axisInfo, false);
}

void StatusBarWidget::setClickToAddMode(const bool active) const { refreshModeLabel(TransformMode::none, {}, active); }

void StatusBarWidget::refreshModeLabel(const TransformMode mode, const QString &axisInfo, const bool clickToAdd) const {
    QString text;
    if (clickToAdd) {
        text = "Mode: Add Point";
        m_modeLabel->setStyleSheet(s_activeStyle);
    }
    else {
        switch (mode) {
        case TransformMode::translate:
            text = "Mode: Translate (G)";
            m_modeLabel->setStyleSheet(s_activeStyle);
            break;
        case TransformMode::rotate:
            text = QString("Mode: Rotate (R)") + (axisInfo.isEmpty()
                                                      ? ""
                                                      : "  Axis: " + axisInfo);
            m_modeLabel->setStyleSheet(s_activeStyle);
            break;
        case TransformMode::scale:
            text = "Mode: Scale (S)";
            m_modeLabel->setStyleSheet(s_activeStyle);
            break;
        case TransformMode::none:
        default:
            text = "Mode: --";
            m_modeLabel->setStyleSheet("");
            break;
        }
    }
    m_modeLabel->setText(text);
}

void StatusBarWidget::setCameraName(const QString &name) const { m_cameraLabel->setText("Camera: " + name); }

void StatusBarWidget::setSelectionCount(const int count) const {
    m_selectionLabel->setText(
        count > 0
            ? QString::number(count) + " selected"
            : ""
    );
}

void StatusBarWidget::setActiveNewPointsTargetName(const QString &name) const {
    m_newPointsTargetLabel->setText(
        name.isEmpty()
            ? "NewPointsTarget: --"
            : "NewPointsTarget: " + name
    );
}
