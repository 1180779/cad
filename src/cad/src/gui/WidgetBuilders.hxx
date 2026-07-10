//
// Created by Radosław Głasek on 24.06.2026
//

#ifndef CAD_WIDGETBUILDERS_HXX
#define CAD_WIDGETBUILDERS_HXX

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <common/ModifierSpinBox.hpp>

#include "components/geometry/VirtualPointPropertiesWidget.hpp"

/// @brief Builders for the common component-panel widgets
namespace widgets {
    inline QLabel* addTitle(QLayout *layout, const QString &text);

    inline QCheckBox* addCheckbox(QLayout *layout, const QString &text, bool checked);

    inline QListWidget* addPointList(
        QLayout *layout,
        const QString &label,
        QAbstractItemView::SelectionMode mode = QAbstractItemView::ExtendedSelection
    );

    inline QPushButton* addButton(QVBoxLayout *layout, const QString &text);

    inline QSpinBox* newSpinBox(int min, int max, int value);

    inline QSpinBox* addSpinBox(
        QLayout *layout,
        const QString &label,
        int min,
        int max,
        int value
    );

    inline QComboBox* addFormComboBox(QFormLayout *form, const QString &label, const QStringList &items);

    inline QSpinBox* addFormSpinBox(QFormLayout *form, const QString &label, int min, int max, int value);

    inline ModifierDoubleSpinBox* newDoubleSpinBox(double min, double max, double value);

    inline ModifierDoubleSpinBox* addDoubleSpinBox(QLayout *layout, const QString &label, int min, int max, int value);

    inline ModifierDoubleSpinBox* addFormDoubleSpinBox(
        QFormLayout *form,
        const QString &label,
        double min,
        double max,
        double value
    );

    inline void addSeparator(QLayout *layout);

    inline VirtualPointPropertiesWidget* addPointProps(QLayout *layout);

    inline QFrame* horizontalLine();

    inline QFrame* horizontalLine(QWidget *parent);

    inline QFrame* horizontalLine(QLayout *layout);
}

namespace widgets {
    // implementation

    inline QLabel* addTitle(QLayout *layout, const QString &text) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto title = new QLabel(text);
        QFont f = title->font();
        f.setBold(true);
        title->setFont(f);
        layout->addWidget(title);
        return title;
    }

    inline QCheckBox* addCheckbox(QLayout *layout, const QString &text, const bool checked) {
        const auto box = new QCheckBox(text);
        box->setChecked(checked);
        layout->addWidget(box);
        return box;
    }

    inline QListWidget* addPointList(
        QLayout *layout,
        const QString &label,
        const QAbstractItemView::SelectionMode mode
    ) {
        layout->addWidget(new QLabel(label));
        const auto list = new QListWidget;
        list->setSelectionMode(mode);
        layout->addWidget(list);
        return list;
    }

    inline QPushButton* addButton(QVBoxLayout *layout, const QString &text) {
        const auto button = new QPushButton(text);
        layout->addWidget(button);
        return button;
    }

    inline QSpinBox* newSpinBox(const int min, const int max, const int value) {
        const auto spin = new QSpinBox;
        spin->setRange(min, max);
        spin->setValue(value);
        return spin;
    }

    inline QSpinBox* addSpinBox(
        QLayout *layout,
        const QString &label,
        const int min,
        const int max,
        const int value
    ) {
        layout->addWidget(new QLabel(label));
        const auto spin = newSpinBox(min, max, value);
        layout->addWidget(spin);
        return spin;
    }

    inline QComboBox* addFormComboBox(QFormLayout *form, const QString &label, const QStringList &items) {
        const auto combo = new QComboBox;
        combo->addItems(items);
        form->addRow(label, combo);
        return combo;
    }

    inline QSpinBox* addFormSpinBox(
        QFormLayout *form,
        const QString &label,
        const int min,
        const int max,
        const int value
    ) {
        const auto spin = new ModifierSpinBox;
        spin->setRange(min, max);
        spin->setValue(value);
        spin->setAlignment(Qt::AlignRight);
        spin->setMinimumWidth(60);
        spin->setMaximumWidth(100);
        form->addRow(label, spin);
        return spin;
    }

    inline ModifierDoubleSpinBox* newDoubleSpinBox(
        const double min,
        const double max,
        const double value
    ) {
        const auto spin = new ModifierDoubleSpinBox;
        spin->setRange(min, max);
        spin->setValue(value);
        spin->setAlignment(Qt::AlignRight);
        spin->setMinimumWidth(60);
        spin->setMaximumWidth(100);
        return spin;
    }

    inline ModifierDoubleSpinBox* addDoubleSpinBox(
        QLayout *layout,
        const QString &label,
        const int min,
        const int max,
        const int value
    ) {
        layout->addWidget(new QLabel(label));
        const auto spin = newDoubleSpinBox(min, max, value);
        layout->addWidget(spin);
        return spin;
    }

    inline ModifierDoubleSpinBox* addFormDoubleSpinBox(
        QFormLayout *form,
        const QString &label,
        const double min,
        const double max,
        const double value
    ) {
        const auto spin = newDoubleSpinBox(min, max, value);
        form->addRow(label, spin);
        return spin;
    }

    inline void addSeparator(QLayout *layout) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        layout->addWidget(sep);
    }

    inline VirtualPointPropertiesWidget* addPointProps(QLayout *layout) {
        const auto props = new VirtualPointPropertiesWidget;
        layout->addWidget(props);
        props->setActive(false);
        return props;
    }

    inline QFrame* horizontalLine() {
        const auto hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setFixedHeight(16);
        hLine->setAutoFillBackground(false);
        return hLine;
    }

    inline QFrame* horizontalLine(QWidget *parent) {
        const auto hLine = horizontalLine();
        hLine->setParent(parent);
        return hLine;
    }

    inline QFrame* horizontalLine(QLayout *layout) {
        const auto hLine = horizontalLine();
        layout->addWidget(hLine);
        return hLine;
    }
}

#endif //CAD_WIDGETBUILDERS_HXX
