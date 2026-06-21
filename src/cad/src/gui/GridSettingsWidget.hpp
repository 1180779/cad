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

    int getGridPlanes() const;

signals :
    void gridPlanesChanged(int planes);

private:
    void onCheckboxToggled();

    QCheckBox *m_xyPlane{nullptr};
    QCheckBox *m_xzPlane{nullptr};
    QCheckBox *m_yzPlane{nullptr};
};

#endif //CAD_GRIDSETTINGSWIDGET_H
