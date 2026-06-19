//
// Created on 3/30/26.
//
#ifndef CAD_MODIFIERSPINBOX_HPP
#define CAD_MODIFIERSPINBOX_HPP
#include <QAbstractButton>
#include <QApplication>
#include <QLineEdit>
#include <QShowEvent>
#include <QSpinBox>

/// Spinboxes that scale steps with modifiers
class ModifierDoubleSpinBox : public QDoubleSpinBox {
    Q_OBJECT

public:
    explicit ModifierDoubleSpinBox(QWidget *parent = nullptr) : QDoubleSpinBox(parent) {
        lineEdit()->setMinimumWidth(50);
    }

    void stepBy(int steps) override {
        if (const auto mods = QApplication::keyboardModifiers();
            mods & Qt::ShiftModifier) {
            steps *= 100;
        }
        QDoubleSpinBox::stepBy(steps);
    }

protected:
    void showEvent(QShowEvent *e) override {
        QDoubleSpinBox::showEvent(e);
        for (auto *btn : findChildren<QAbstractButton*>()) {
            btn->setMaximumWidth(16);
        }
    }
};

class ModifierSpinBox : public QSpinBox {
    Q_OBJECT

public:
    explicit ModifierSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {
        lineEdit()->setMinimumWidth(50);
    }

    void stepBy(int steps) override {
        if (const auto mods = QApplication::keyboardModifiers();
            mods & Qt::ShiftModifier) {
            steps *= 100;
        }
        QSpinBox::stepBy(steps);
    }

protected:
    void showEvent(QShowEvent *e) override {
        QSpinBox::showEvent(e);
        for (auto *btn : findChildren<QAbstractButton*>()) {
            btn->setMaximumWidth(16);
        }
    }
};
#endif //CAD_MODIFIERSPINBOX_HPP