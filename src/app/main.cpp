#include <QApplication>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QDebug>

#include "OpenGLWidget.h"

int main(int argc, char* argv[])
{
    GLSetDefaults();
    QApplication a(argc, argv);

    QWidget window;
    window.setMinimumSize(QSize(500, 500));

    constexpr int rightWidgetsMaxSize = 300;

    auto layout = new QHBoxLayout(&window);
    auto rightControlsLayout = new QVBoxLayout;
    rightControlsLayout->setAlignment(Qt::AlignTop);
    auto leftControlsLayout = new QVBoxLayout;
    layout->addLayout(leftControlsLayout);
    layout->addLayout(rightControlsLayout);

    // Ellipse Parameters group of widgets
    auto ellipseParametersGroup = new QGroupBox("Ellipse Parameters");
    ellipseParametersGroup->setMaximumWidth(rightWidgetsMaxSize);

    auto ellipseAParameterLayout = new QHBoxLayout;
    auto ellipseAParameterLabel = new QLabel("a");
    auto ellipseAParameterEdit = new QLineEdit;
    ellipseAParameterLayout->addWidget(ellipseAParameterLabel);
    ellipseAParameterLayout->addWidget(ellipseAParameterEdit);

    auto ellipseBParameterLayout = new QHBoxLayout;
    auto ellipseBParameterLabel = new QLabel("b");
    auto ellipseBParameterEdit = new QLineEdit;
    ellipseBParameterLayout->addWidget(ellipseBParameterLabel);
    ellipseBParameterLayout->addWidget(ellipseBParameterEdit);

    auto ellipseCParameterLayout = new QHBoxLayout;
    auto ellipseCParameterLabel = new QLabel("c");
    auto ellipseCParameterEdit = new QLineEdit;
    ellipseCParameterLayout->addWidget(ellipseCParameterLabel);
    ellipseCParameterLayout->addWidget(ellipseCParameterEdit);

    auto ellipseParametersLayout = new QVBoxLayout;
    ellipseParametersLayout->addLayout(ellipseAParameterLayout);
    ellipseParametersLayout->addLayout(ellipseBParameterLayout);
    ellipseParametersLayout->addLayout(ellipseCParameterLayout);
    ellipseParametersGroup->setLayout(ellipseParametersLayout);

    // Adaptive rendering group of widgets
    auto adaptiveRenderingGroup = new QGroupBox("Adaptive rendering");
    adaptiveRenderingGroup->setMaximumWidth(rightWidgetsMaxSize);

    auto adaptiveRenderingLayout = new QHBoxLayout;
    auto adaptiveRenderingSquareSize = new QLabel("square size");
    auto adaptiveRenderingSquareSizeEdit = new QLineEdit;
    adaptiveRenderingLayout->addWidget(adaptiveRenderingSquareSize);
    adaptiveRenderingLayout->addWidget(adaptiveRenderingSquareSizeEdit);
    adaptiveRenderingGroup->setLayout(adaptiveRenderingLayout);

    auto glWidget = new OpenGLWidget;
    leftControlsLayout->addWidget(glWidget);

    rightControlsLayout->addWidget(ellipseParametersGroup, 0, Qt::AlignTop);
    rightControlsLayout->addWidget(adaptiveRenderingGroup, 0, Qt::AlignTop);

    window.show();
    return QApplication::exec();
}
