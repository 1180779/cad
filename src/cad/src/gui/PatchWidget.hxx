//
// Created by Radosław Głasek on 02.07.2026
//

#ifndef CAD_PATCHWIDGET_HXX
#define CAD_PATCHWIDGET_HXX

#include <QCheckBox>
#include <QSpinBox>

#include "ComponentWidget.hpp"
#include "../components/PatchComponent.hxx"

/// @brief Property panel for a joined Bézier patch
class PatchWidget final : public ComponentWidget {
    Q_OBJECT

public:
    explicit PatchWidget(PatchComponent *patch, const QString &title, QWidget *parent = nullptr);

private:
    void subdivisionUChanged(int value);

    void subdivisionVChanged(int value);

    PatchComponent *m_patch;
    QCheckBox *m_showNetCheckbox{};
    QSpinBox *m_divisionsUSpin{};
    QSpinBox *m_divisionsVSpin{};

    /// @brief Last committed subdivision values, for building the undo revert
    int m_lastDivisionsU;
    int m_lastDivisionsV;
};

#endif //CAD_PATCHWIDGET_HXX
