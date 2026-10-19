//
// Created by Radosław Głasek on 19.10.2026
//

#ifndef WIDGETBUILDERS_HXX
#define WIDGETBUILDERS_HXX

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <optional>

#include <common/ModifierSpinBox.hpp>


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

    inline QSpinBox* newSpinBox(int min, int max, int value, QWidget *parent);

    inline QSpinBox* addSpinBox(
        QLayout *layout,
        const QString &label,
        int min,
        int max,
        int value
    );

    inline QComboBox* addFormComboBox(QFormLayout *form, const QString &label, const QStringList &items, QWidget * parent = nullptr);

    inline QSpinBox* addFormSpinBox(QFormLayout *form, const QString &label, int min, int max, int value);

    inline ModifierDoubleSpinBox* newDoubleSpinBox(double min, double max, double value, QWidget *parent = nullptr);
    
    inline ModifierDoubleSpinBox* newDoubleSpinBox(
        double min,
        double max,
        double value,
        double step,
        QWidget * parent = nullptr
    );

    inline ModifierDoubleSpinBox* addDoubleSpinBox(QLayout *layout, const QString &label, int min, int max, int value);

    inline ModifierDoubleSpinBox* addFormDoubleSpinBox(
        QFormLayout *form,
        const QString &label,
        double min,
        double max,
        double value
    );

    inline void addSeparator(QLayout *layout);

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

    inline QSpinBox* newSpinBox(const int min, const int max, const int value, QWidget *const parent = nullptr) {
        const auto spin = new QSpinBox(parent);
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

    inline QCheckBox* addFormCheckbox(QFormLayout *form, const QString &label, const bool checked = false, QWidget *const parent = nullptr)
    {
        const auto checkbox = new QCheckBox(label, parent);
        checkbox->setChecked(checked);
        form->addRow(checkbox);
        return checkbox;
    }
    
    inline QComboBox* addFormComboBox(QFormLayout *form, const QString &label, const QStringList &items, QWidget *const parent) {
        const auto combo = new QComboBox(parent);
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
    
    inline QPushButton* addFormPushButton(QFormLayout *form, const QString &label, QWidget *const parent = nullptr)
    {
        const auto pushbutton = new QPushButton(parent);
        form->addRow(label, pushbutton);
        return pushbutton;
    }
    
    
    inline QSpinBox* addFormSpinBox(
        QFormLayout *form,
        const QString &label,
        const int min,
        const int max,
        const int value,
        const int step)
    {
        const auto spin = addFormSpinBox(form, label, min, max, value);
        spin->setSingleStep(step);
        return spin;
    }

    inline ModifierDoubleSpinBox* newDoubleSpinBox(
        const double min,
        const double max,
        const double value,
        QWidget *const parent
    ) {
        const auto spin = new ModifierDoubleSpinBox(parent);
        spin->setRange(min, max);
        spin->setValue(value);
        spin->setAlignment(Qt::AlignRight);
        spin->setMinimumWidth(60);
        spin->setMaximumWidth(100);
        return spin;
    }
    
    inline ModifierDoubleSpinBox* newDoubleSpinBox(
        const double min,
        const double max,
        const double value,
        const double step,
        QWidget *const parent
    ) {
        const auto spin = newDoubleSpinBox(min, max, value, parent);
        spin->setDecimals(step > 0.1 ? 1 : 2);
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
    
    inline ModifierDoubleSpinBox* addFormDoubleSpinBox(
        QFormLayout *form,
        const QString &label,
        const double min,
        const double max,
        const double value,
        const double step,
        QWidget *const parent = nullptr
    ) {
        const auto spin = newDoubleSpinBox(min, max, value, step, parent);
        form->addRow(label, spin);
        return spin;
    }

    inline void addSeparator(QLayout *layout) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        layout->addWidget(sep);
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

namespace widgets {
    struct SpinBoxConfig {
        std::optional<int> minimum;
        std::optional<int> maximum;
        std::optional<int> value;
        std::optional<int> step;
    };

    struct DoubleSpinBoxConfig {
        std::optional<double> minimum;
        std::optional<double> maximum;
        std::optional<double> value;
        std::optional<double> step;
        std::optional<int> decimals;
    };

    struct FormBuilderConfig {
        QWidget* parent = nullptr;
        SpinBoxConfig spinBox;
        DoubleSpinBoxConfig doubleSpinBox;
        std::optional<int> listMaximumHeight;
    };

    struct LayoutBuilderConfig {
        QWidget* parent = nullptr;
        std::optional<bool> checked;
        std::optional<int> listMaximumHeight;
    };

    // Per-call values take precedence over the builder's stored defaults.
    class FormBuilder {
    public:
        explicit FormBuilder(QFormLayout* form, const FormBuilderConfig& config = {})
            : m_form(form), m_config(config) {}

        [[nodiscard]] QSpinBox* addSpinBox(const QString& label, const SpinBoxConfig& options = {}) const {
            const auto& defaults = m_config.spinBox;
            auto* spin = new ModifierSpinBox(m_config.parent);
            spin->setRange(options.minimum.value_or(defaults.minimum.value_or(spin->minimum())),
                           options.maximum.value_or(defaults.maximum.value_or(spin->maximum())));
            if (const auto step = options.step ? options.step : defaults.step) {
                spin->setSingleStep(*step);
            }
            if (const auto value = options.value ? options.value : defaults.value) {
                spin->setValue(*value);
            }
            spin->setAlignment(Qt::AlignRight);
            spin->setMinimumWidth(60);
            spin->setMaximumWidth(100);
            m_form->addRow(label, spin);
            return spin;
        }

        [[nodiscard]] ModifierDoubleSpinBox* addDoubleSpinBox(const QString& label, const DoubleSpinBoxConfig& options = {}) const {
            const auto& defaults = m_config.doubleSpinBox;
            auto* spin = new ModifierDoubleSpinBox(m_config.parent);
            if (const auto decimals = options.decimals ? options.decimals : defaults.decimals) {
                spin->setDecimals(*decimals);
            }
            spin->setRange(options.minimum.value_or(defaults.minimum.value_or(spin->minimum())),
                           options.maximum.value_or(defaults.maximum.value_or(spin->maximum())));
            if (const auto step = options.step ? options.step : defaults.step) {
                spin->setSingleStep(*step);
            }
            if (const auto value = options.value ? options.value : defaults.value) {
                spin->setValue(*value);
            }
            spin->setAlignment(Qt::AlignRight);
            spin->setMinimumWidth(60);
            spin->setMaximumWidth(100);
            m_form->addRow(label, spin);
            return spin;
        }

        QCheckBox* addCheckBox(const QString& text, const bool checked = false) const { // NOLINT(*-use-nodiscard)
            return addFormCheckbox(m_form, text, checked, m_config.parent);
        }

        QComboBox* addComboBox(const QString& label, const QStringList& items) const { // NOLINT(*-use-nodiscard)
            return addFormComboBox(m_form, label, items, m_config.parent);
        }

        QPushButton* addButton(const QString& label) const { // NOLINT(*-use-nodiscard)
            return addFormPushButton(m_form, label, m_config.parent);
        }

        QListWidget* addList(const std::optional<int> maximumHeight = std::nullopt) const { // NOLINT(*-use-nodiscard)
            auto* list = new QListWidget(m_config.parent);
            if (const int height = maximumHeight.value_or(m_config.listMaximumHeight.value_or(0)); height > 0) {
                list->setMaximumHeight(height);
            }
            m_form->addRow(list);
            return list;
        }

    private:
        QFormLayout* m_form;
        FormBuilderConfig m_config;
    };

    class LayoutBuilder {
    public:
        explicit LayoutBuilder(QLayout* layout, const LayoutBuilderConfig& config = {})
            : m_layout(layout), m_config(config) {}

        QPushButton* addButton(const QString& text) const { // NOLINT(*-use-nodiscard)
            auto* button = new QPushButton(text, m_config.parent);
            m_layout->addWidget(button);
            return button;
        }

        QCheckBox* addCheckBox(const QString& text, const std::optional<bool> checked = std::nullopt) const { // NOLINT(*-use-nodiscard)
            auto* box = new QCheckBox(text, m_config.parent);
            box->setChecked(checked.value_or(m_config.checked.value_or(false)));
            m_layout->addWidget(box);
            return box;
        }

        QListWidget* addList(const std::optional<int> maximumHeight = std::nullopt) const { // NOLINT(*-use-nodiscard)
            auto* list = new QListWidget(m_config.parent);
            if (const int height = maximumHeight.value_or(m_config.listMaximumHeight.value_or(0)); height > 0) {
                list->setMaximumHeight(height);
            }
            m_layout->addWidget(list);
            return list;
        }

    private:
        QLayout* m_layout;
        LayoutBuilderConfig m_config;
    };

}

#endif //WIDGETBUILDERS_HXX
