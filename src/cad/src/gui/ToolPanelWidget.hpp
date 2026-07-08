//
// Created by Radosław Głasek on 21.06.2026
//

#ifndef CAD_TOOLPANELWIDGET_HPP
#define CAD_TOOLPANELWIDGET_HPP

#include <concepts>

#include <QLayout>
#include <QString>
#include <QWidget>

/// @brief Base class for right-side tool panels. Carries a display name used by
/// <tt>ToolPanelBar</tt>
class ToolPanelWidget : public QWidget {
    Q_OBJECT

public:
    explicit ToolPanelWidget(QString name, QWidget *parent = nullptr)
    : QWidget(parent),
      m_name(std::move(name)) {}

    [[nodiscard]] const QString& panelName() const {
        return m_name;
    }

protected:
    template <typename TLayout = QVBoxLayout> requires std::derived_from<TLayout, QLayout>
    TLayout* createLayout(Qt::AlignmentFlag alignment = Qt::AlignTop) {
        const auto layout = new TLayout(this);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setAlignment(alignment);
        return layout;
    }

private:
    QString m_name;
};

#endif //CAD_TOOLPANELWIDGET_HPP
