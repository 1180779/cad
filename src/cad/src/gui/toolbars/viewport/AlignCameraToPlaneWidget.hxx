//
// Created by Radosław Głasek on 10.07.2026
//

#ifndef CAD_ALIGNCAMERATOPLANEWIDGET_HXX
#define CAD_ALIGNCAMERATOPLANEWIDGET_HXX

#include <array>
#include <QGroupBox>

#include "gui/WidgetBuilders.hxx"
#include "ViewportTypes.hpp"

class AlignCameraToPlaneWidget final : public QWidget {
    Q_OBJECT

public:
    explicit AlignCameraToPlaneWidget(QWidget *parent);

signals:
    void alignToPlaneRequested(Plane plane);

private:
    std::array<QPushButton*, PlaneCount> m_pushButtons{};
};

// ReSharper disable CppDFAMemoryLeak

inline AlignCameraToPlaneWidget::AlignCameraToPlaneWidget(QWidget *parent)
: QWidget{parent} {
    const auto outerLayout = new QVBoxLayout(this);
    outerLayout->setAlignment(Qt::AlignTop);
    const auto groupBox = new QGroupBox("Align camera to planes");
    const auto groupLayout = new QVBoxLayout(groupBox);
    for (std::size_t i = 0; i < PlaneCount; i++) {
        const auto plane = static_cast<Plane>(i);
        m_pushButtons[i] = widgets::addButton(groupLayout, toString(plane));
        const auto slot = [this, plane] {
            emit alignToPlaneRequested(plane);
        };
        connect(m_pushButtons[i], &QPushButton::clicked, this, slot);
    }
    outerLayout->addWidget(groupBox);
}

// ReSharper restore CppDFAMemoryLeak

#endif //CAD_ALIGNCAMERATOPLANEWIDGET_HXX
