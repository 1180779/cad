//
// Created on 3/25/26.
//

#ifndef CAD_GRIDSETTINGSWIDGET_H
#define CAD_GRIDSETTINGSWIDGET_H

#include <QCheckBox>
#include <QWidget>

class GridSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit GridSettingsWidget(QWidget *parent = nullptr);

    /// @brief XY|XZ|YZ bitmask for the infinite grid planes (bit 0=XY, 1=XZ, 2=YZ)
    int getGridPlanes() const;

    /// @brief X|Y|Z bitmask for the infinite world axes (bit 0=X, 1=Y, 2=Z)
    int getAxesMask() const;

    /// @brief Whether the distance LOD fade is enabled for grid + axes
    bool getLodFade() const;

signals :
    void gridPlanesChanged(int planes);

    void axesMaskChanged(int mask);

    void lodFadeChanged(bool enabled);

private:
    void onCheckboxToggled();

    QCheckBox *m_xyPlane{nullptr};
    QCheckBox *m_xzPlane{nullptr};
    QCheckBox *m_yzPlane{nullptr};

    QCheckBox *m_xAxis{nullptr};
    QCheckBox *m_yAxis{nullptr};
    QCheckBox *m_zAxis{nullptr};

    QCheckBox *m_lodFade{nullptr};
};

#endif //CAD_GRIDSETTINGSWIDGET_H
