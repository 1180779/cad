//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_VIEWPORTPANELWIDGET_HPP
#define CAD_VIEWPORTPANELWIDGET_HPP

#include "AlignCameraToPlaneWidget.hxx"
#include "GridSettingsWidget.hpp"
#include "../ToolPanelWidget.hpp"

class ViewportPanelWidget final : public ToolPanelWidget {
    Q_OBJECT

public:
    explicit ViewportPanelWidget(QWidget *parent = nullptr);

    [[nodiscard]] const GridSettingsWidget* gridSettingsWidget() const;

    [[nodiscard]] const QComboBox* pivotCombo() const;

    [[nodiscard]] const QComboBox* coordSpaceCombo() const;

    [[nodiscard]] const AlignCameraToPlaneWidget* alignCameraWidget() const;

private:
    GridSettingsWidget *m_gridSettings;
    AlignCameraToPlaneWidget *m_alignCameraWidget;
    QComboBox *m_pivotCombo;
    QComboBox *m_coordSpaceCombo;
};

#endif //CAD_VIEWPORTPANELWIDGET_HPP
