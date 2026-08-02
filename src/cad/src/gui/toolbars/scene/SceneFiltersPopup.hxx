//
// Created by Radosław Głasek on 10.07.2026
//

#ifndef CAD_SCENEFILTERSPOPUP_HXX
#define CAD_SCENEFILTERSPOPUP_HXX

#include <ranges>
#include <QWidget>
#include "components/ComponentChecker.hxx"
#include "gui/WidgetBuilders.hxx"
#include "components/AllComponents.hxx"

// ReSharper disable CppDFAMemoryLeak

/// @brief Popup with per-component-type checkboxes filtering the scene
/// hierarchy list
/// @note no checked boxes means no filtering
class SceneFiltersPopup final : public QWidget {
    Q_OBJECT

public:
    explicit SceneFiltersPopup(QWidget *parent = nullptr)
    : QWidget(parent, Qt::Popup) {
        // ReSharper disable once CppDFAMemoryLeak
        const auto layout = new QVBoxLayout(this);
        const auto clearButton = widgets::addButton(layout, "Clear");
        connect(clearButton, &QPushButton::clicked, this, &SceneFiltersPopup::clearFilters);
        const auto selectAllButton = widgets::addButton(layout, "Select all");
        connect(selectAllButton, &QPushButton::clicked, this, &SceneFiltersPopup::selectAllFilters);
        widgets::horizontalLine(layout);
        addRow<CameraComponent>(layout, "Camera");
        addRow<CursorComponent>(layout, "Cursor");
        widgets::horizontalLine(layout);
        addRow<PointComponent>(layout, "Point");
        addRow<TorusComponent>(layout, "Torus");
        addRow<BezierC0Component>(layout, "Bezier C0");
        addRow<BezierC2Component>(layout, "Bezier C2");
        addRow<InterpC2Component>(layout, "Interpolating C2");
        addRow<PatchC0Component>(layout, "Patch C0");
        addRow<PatchC2Component>(layout, "Patch C2");
        addRow<GregoryComponent>(layout, "Gregory patch");
        addRow<IntersectionCurveComponent>(layout, "Intersection curve");
    }

    /// @brief Builds the filter list from the currently checked boxes
    [[nodiscard]] ComponentFilters currentFilters() const {
        ComponentFilters filters;
        for (const auto &[box, make] : m_rows) {
            if (box->isChecked()) {
                filters.push_back(make());
            }
        }
        return filters;
    }

signals:
    void filtersChanged();

private slots:
    void clearFilters();

    void selectAllFilters();

private:
    using CheckerFactory = std::unique_ptr<IComponentChecker> (*)();

    template <typename T>
    void addRow(QVBoxLayout *layout, const QString &label) {
        const auto box = widgets::addCheckbox(layout, label, false);
        connect(box, &QCheckBox::toggled, this, &SceneFiltersPopup::filtersChanged);
        m_rows.emplace_back(
            box,
            +[]() -> std::unique_ptr<IComponentChecker> {
                return std::make_unique<ComponentChecker<T>>();
            }
        );
    }

    std::vector<std::pair<QCheckBox*, CheckerFactory>> m_rows{};
};

inline void SceneFiltersPopup::selectAllFilters() {
    bool changed = false;
    for (const auto box : m_rows | std::views::keys) {
        const QSignalBlocker blocker(box);
        changed |= !box->isChecked();
        box->setChecked(true);
    }
    if (changed) {
        emit filtersChanged();
    }
}

inline void SceneFiltersPopup::clearFilters() {
    bool changed = false;
    for (const auto box : m_rows | std::views::keys) {
        const QSignalBlocker blocker(box);
        changed |= box->isChecked();
        box->setChecked(false);
    }
    if (changed) {
        emit filtersChanged();
    }
}

// ReSharper restore CppDFAMemoryLeak

#endif //CAD_SCENEFILTERSPOPUP_HXX
