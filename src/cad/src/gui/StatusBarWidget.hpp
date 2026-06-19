//
// Created on 5/4/26.
//

#ifndef CAD_STATUSBARWIDGET_HPP
#define CAD_STATUSBARWIDGET_HPP

#include <QLabel>
#include <QWidget>

#include "ViewportTypes.hpp"

/// @brief Vim-like status bar widget with the most relevant information available at a glance
class StatusBarWidget final : public QWidget {
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget *parent = nullptr);

    static constexpr int s_barHeight = 24;
    static constexpr auto s_barStyle =
        "background-color: #ffffff; color: #000000; font-family: monospace; font-size: 12px;";
    static constexpr auto s_separatorStyle =
        "color: #000000;";
    static constexpr auto s_activeStyle =
        "color: #ff00ff; font-weight: bold;";

public
    slots  :

    

    void setTransformMode(TransformMode mode, const QString &axisInfo) const;

    void setClickToAddMode(bool active) const;

    void setCameraName(const QString &name) const;

    void setSelectionCount(int count) const;

    void setActiveNewPointsTargetName(const QString &name) const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_modeLabel;
    QLabel *m_cameraLabel;
    QLabel *m_newPointsTargetLabel;
    QLabel *m_selectionLabel;

    void refreshModeLabel(TransformMode mode, const QString &axisInfo, bool clickToAdd) const;
};
#endif //CAD_STATUSBARWIDGET_HPP
