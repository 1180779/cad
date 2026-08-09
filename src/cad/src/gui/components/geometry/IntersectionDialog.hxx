//
// Created by Radosław Głasek on 02.08.2026
//

#ifndef CAD_INTERSECTIONDIALOG_HXX
#define CAD_INTERSECTIONDIALOG_HXX

#include <QCheckBox>
#include <QDialog>

#include <common/ModifierSpinBox.hpp>

#include <cad_math/Common.hpp>

/// @brief Parameters for one run of the intersection tracer
struct IntersectionParams {
    /// @brief Arclength between consecutive traced points
    cadm::cadf step = 0.01f;

    int maxPoints = 2000;

    /// @brief Seed from the point nearest the 3D cursor instead of searching
    /// the whole domain
    bool useCursor = false;
};

/// @brief Modal dialog collecting @ref IntersectionParams before tracing
class IntersectionDialog final : public QDialog {
    Q_OBJECT

public:
    /// @param cursorAvailable whether the scene has an active 3D cursor; the
    /// cursor option is disabled without one
    /// @param parent QWidget base widget parent
    explicit IntersectionDialog(bool cursorAvailable, QWidget *parent = nullptr);

    [[nodiscard]] IntersectionParams params() const;

private:
    ModifierDoubleSpinBox *m_step{};
    QSpinBox *m_maxPoints{};
    QCheckBox *m_useCursor{};
};

#endif //CAD_INTERSECTIONDIALOG_HXX
