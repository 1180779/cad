//
// Created by Radosław Głasek on 24.06.2026
//

#ifndef CAD_WIDGETBUILDERS_HXX
#define CAD_WIDGETBUILDERS_HXX

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "VirtualPointPropertiesWidget.hpp"

/// @brief Builders for the common component-panel widgets
namespace widgets {
    inline void addTitle(QVBoxLayout *layout, const QString &text) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto title = new QLabel(text);
        QFont f = title->font();
        f.setBold(true);
        title->setFont(f);
        layout->addWidget(title);
    }

    inline QCheckBox* addCheckbox(QVBoxLayout *layout, const QString &text, const bool checked) {
        const auto box = new QCheckBox(text);
        box->setChecked(checked);
        layout->addWidget(box);
        return box;
    }

    inline QListWidget* addPointList(
        QVBoxLayout *layout,
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

    inline void addSeparator(QVBoxLayout *layout) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        layout->addWidget(sep);
    }

    inline VirtualPointPropertiesWidget* addPointProps(QVBoxLayout *layout) {
        const auto props = new VirtualPointPropertiesWidget;
        layout->addWidget(props);
        props->setActive(false);
        return props;
    }
}

#endif //CAD_WIDGETBUILDERS_HXX
