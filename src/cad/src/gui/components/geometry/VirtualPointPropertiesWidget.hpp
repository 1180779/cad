//
// Created on 6/21/26.
//

#ifndef CAD_VIRTUALPOINTPROPERTIESWIDGET_HPP
#define CAD_VIRTUALPOINTPROPERTIESWIDGET_HPP

#include <QWidget>
#include <cad_math/Vec3.hpp>

class QLabel;
class ModifierDoubleSpinBox;

/// @brief Reusable X/Y/Z coordinate editor for a single "virtual" point
///
/// @details Purely presentational: shows a caption plus three spinboxes and emits
/// coordinateEdited() on a user edit. It holds no notion of what the point is
class VirtualPointPropertiesWidget final : public QWidget {
    Q_OBJECT

public:
    explicit VirtualPointPropertiesWidget(QWidget *parent = nullptr);

    /// @brief Set the caption shown above the spinboxes
    void setCaption(const QString &text) const;

    /// @brief Load @p pos into the spinboxes without emitting coordinateEdited()
    void setPosition(cadm::Vec3 pos);

    /// @brief Enable/disable the caption and spinboxes together
    void setActive(bool active) const;

signals:
    /// @brief Emitted on a user edit, carrying the full current X/Y/Z vector
    void coordinateEdited(cadm::Vec3 pos);

private:
    cadm::Vec3 currentValue() const;

    QLabel *m_caption{};
    ModifierDoubleSpinBox *m_x{};
    ModifierDoubleSpinBox *m_y{};
    ModifierDoubleSpinBox *m_z{};

    bool m_refreshing = false;
};

#endif //CAD_VIRTUALPOINTPROPERTIESWIDGET_HPP
