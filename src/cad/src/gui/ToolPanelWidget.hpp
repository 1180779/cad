#ifndef CAD_TOOLPANELWIDGET_HPP
#define CAD_TOOLPANELWIDGET_HPP

#include <QString>
#include <QWidget>

/// @brief Base class for right-side tool panels. Carries a display name used by ToolPanelBar
class ToolPanelWidget : public QWidget {
    Q_OBJECT

public:
    explicit ToolPanelWidget(QString name, QWidget *parent = nullptr) : QWidget(parent), m_name(std::move(name)) {}

    [[nodiscard]] const QString& panelName() const {
        return m_name;
    }

private:
    QString m_name;
};

#endif
