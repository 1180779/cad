//
// Created by Radosław Głasek on 02.07.2026
//

#ifndef CAD_PATCHCREATORDIALOG_HXX
#define CAD_PATCHCREATORDIALOG_HXX

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>

#include <common/ModifierSpinBox.hpp>

#include "PatchGeometry.hxx"

class PatchCreatorDialog;

namespace aliases {
    // ReSharper disable once CppInconsistentNaming
    using PCDialog = PatchCreatorDialog;
}

/// @brief Non-modal frameless dialog collecting parameters for a new joined
/// Bézier patch; the viewport stays interactive for the live preview
class PatchCreatorDialog final : public QDialog {
    Q_OBJECT

public:
    explicit PatchCreatorDialog(bool c2, QWidget *parent = nullptr);

    /// @brief Current parameters as configured in the dialog
    [[nodiscard]] patchgen::PatchCreateParams params() const;

signals :
    /// @brief Emitted whenever any field changes (and once on construction) so
    /// a caller can drive a live preview

    void paramsChanged(patchgen::PatchCreateParams params);

    /// @brief Emitted when the "Show control net" toggle changes, to drive the
    /// preview's net visibility
    void showNetChanged(bool visible);

    /// @brief Emitted when the "Hide other objects" toggle changes, to isolate
    /// the preview in the viewport
    void hideSceneChanged(bool hidden);

private:
    void updateForType() const;

    bool m_c2;
    QComboBox *m_typeCombo{};
    QSpinBox *m_countX{};
    QSpinBox *m_countY{};
    ModifierDoubleSpinBox *m_width{};
    ModifierDoubleSpinBox *m_length{};
    ModifierDoubleSpinBox *m_radius{};
    ModifierDoubleSpinBox *m_height{};
    QCheckBox *m_showNet{};
    QCheckBox *m_hideScene{};
};

#endif //CAD_PATCHCREATORDIALOG_HXX
