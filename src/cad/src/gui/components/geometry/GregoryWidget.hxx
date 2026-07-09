//
// Created by Radosław Głasek on 09.07.2026
//

#ifndef CAD_GREGORYWIDGET_HXX
#define CAD_GREGORYWIDGET_HXX

#include <vector>

#include <QCheckBox>
#include <QSpinBox>

#include "../ComponentWidget.hpp"
#include "../../../components/geometry/GregoryComponent.hxx"

class GregoryWidget final : public ComponentWidget {
    Q_OBJECT

public:
    explicit GregoryWidget(GregoryComponent *gregory, QWidget *parent = nullptr);

private:
    /// @brief Commit one net's U or V subdivision change as an undoable edit
    void subdivisionChanged(
        void (GregoryComponent::*setter)(int, int),
        std::vector<int> &lastValues,
        const std::vector<QSpinBox*> &spins,
        int net,
        int value
    );

    /// @brief Apply the master value to every net's U and V as one undo entry
    void applyUniform(int value);

    /// @brief Enable/disable per-net spinboxes vs the master spinbox
    void uniformToggled(bool checked);

    GregoryComponent *m_gregory;
    QCheckBox *m_showVectorsCheckbox{};

    QCheckBox *m_uniformCheckbox{};
    QSpinBox *m_uniformSpin{};

    /// @brief Per-net subdivision spinboxes
    std::vector<QSpinBox*> m_divisionsUSpins;
    std::vector<QSpinBox*> m_divisionsVSpins;

    /// @brief Last committed subdivision values, for building the undo revert
    std::vector<int> m_lastDivisionsU;
    std::vector<int> m_lastDivisionsV;
};

#endif //CAD_GREGORYWIDGET_HXX
