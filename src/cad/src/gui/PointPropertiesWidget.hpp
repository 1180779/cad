//
// Created on 3/31/26.
//
#ifndef CAD_POINTPROPERTIESWIDGET_HPP
#define CAD_POINTPROPERTIESWIDGET_HPP
#include <common/ModifierSpinBox.hpp>
#include "../PointRegistry.hpp"

class Scene;
class CommandStack;

class PointPropertiesWidget : public QWidget {
    Q_OBJECT

public:
    explicit PointPropertiesWidget(PointRegistry *registry, QWidget *parent = nullptr);

    /// @brief Sets the context required for executing undoable commands
    /// @param scene The active scene containing the points
    /// @param stack The command stack to push coordinate modification commands onto
    void setCommandContext(Scene *scene, CommandStack *stack) {
        m_scene = scene;
        m_commandStack = stack;
    }

    void setPoint(PointHandle handle);

    void refresh();

    static constexpr double s_coordMin = std::numeric_limits<double>::lowest();
    static constexpr double s_coordMax = std::numeric_limits<double>::max();
    static constexpr double s_coordStep = 0.1;
    static constexpr int s_widgetWidth = 100;
signals  :
    

    void propertyChanged();

private
slots  :
    

    void onXChanged(double value);

    void onYChanged(double value);

    void onZChanged(double value);

private:
    /// axis: 0=x, 1=y, 2=z
    void applyCoordEdit(int axis, double value);

private:
    PointRegistry *m_registry = nullptr;
    Scene *m_scene = nullptr;
    CommandStack *m_commandStack = nullptr;
    PointHandle m_handle = InvalidPointHandle;
    ModifierDoubleSpinBox *m_x{};
    ModifierDoubleSpinBox *m_y{};
    ModifierDoubleSpinBox *m_z{};
};
#endif //CAD_POINTPROPERTIESWIDGET_HPP
