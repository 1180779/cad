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

#include "VirtualPointPropertiesWidget.hpp"

/// @brief Builders for the common component-panel widgets
namespace widgets {
    inline void addTitle(QLayout *layout, const QString &text) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto title = new QLabel(text);
        QFont f = title->font();
        f.setBold(true);
        title->setFont(f);
        layout->addWidget(title);
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
        const QAbstractItemView::SelectionMode mode = QAbstractItemView::ExtendedSelection
    ) {
        layout->addWidget(new QLabel(label));
        const auto list = new QListWidget;
        list->setSelectionMode(mode);
        list->setMaximumHeight(120);
        layout->addWidget(list);
        return list;
    }

    inline QPushButton* addButton(QVBoxLayout *layout, const QString &text) {
        const auto button = new QPushButton(text);
        layout->addWidget(button);
        return button;
    }

    inline QSpinBox* addSpinBox(
        QLayout *layout,
        const QString &label,
        const int min,
        const int max,
        const int value
    ) {
        layout->addWidget(new QLabel(label));
        const auto spin = new QSpinBox;
        spin->setRange(min, max);
        spin->setValue(value);
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

    inline ModifierDoubleSpinBox* addFormDoubleSpinBox(
        QFormLayout *form,
        const QString &label,
        const double min,
        const double max,
        const double value
    ) {
        const auto spin = new ModifierDoubleSpinBox;
        spin->setRange(min, max);
        spin->setValue(value);
        spin->setAlignment(Qt::AlignRight);
        spin->setMaximumWidth(100);
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
}

#endif //CAD_WIDGETBUILDERS_HXX
