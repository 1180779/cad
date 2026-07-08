//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_VIEWPORTPANELWIDGET_HPP
#define CAD_VIEWPORTPANELWIDGET_HPP

#include <QComboBox>

#include "GridSettingsWidget.hpp"
#include "../ToolPanelWidget.hpp"

class ViewportPanelWidget final : public ToolPanelWidget {
    Q_OBJECT

public:
    explicit ViewportPanelWidget(QWidget *parent = nullptr);

    [[nodiscard]] GridSettingsWidget* gridSettingsWidget() const;

    [[nodiscard]] QComboBox* pivotCombo() const;

    [[nodiscard]] QComboBox* coordSpaceCombo() const;

private:
    GridSettingsWidget *m_gridSettings;
    QComboBox *m_pivotCombo;
    QComboBox *m_coordSpaceCombo;
};

#endif //CAD_VIEWPORTPANELWIDGET_HPP
