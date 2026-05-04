//
// Created on 3/30/26.
//

#ifndef CAD_MODIFIERSPINBOX_HPP
#define CAD_MODIFIERSPINBOX_HPP

#include <QApplication>
#include <QDoubleSpinBox>
#include <QSpinBox>

// Spinboxes that scale steps with modifiers

class ModifierDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    explicit ModifierDoubleSpinBox(QWidget *parent = nullptr)
        : QDoubleSpinBox(parent)
    {
    }

    void stepBy(int steps) override
    {
        if (const auto mods = QApplication::keyboardModifiers(); mods & Qt::ShiftModifier)
            steps *= 100;
        QDoubleSpinBox::stepBy(steps);
    }
};

class ModifierSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit ModifierSpinBox(QWidget *parent = nullptr)
        : QSpinBox(parent)
    {
    }

    void stepBy(int steps) override
    {
        if (const auto mods = QApplication::keyboardModifiers(); mods & Qt::ShiftModifier)
            steps *= 100;
        QSpinBox::stepBy(steps);
    }
};

#endif //CAD_MODIFIERSPINBOX_HPP
