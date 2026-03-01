#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>

#include "OpenGLWidget.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QWidget window;
    window.setMinimumSize(QSize(500, 500));

    constexpr int rightWidgetsMaxSize = 300;

    QHBoxLayout layout(&window);
    QVBoxLayout rightControlsLayout;
    rightControlsLayout.setAlignment(Qt::AlignTop);
    QVBoxLayout leftControlsLayout;
    layout.addLayout(&leftControlsLayout);
    layout.addLayout(&rightControlsLayout);

    // Ellipse Parameters group of widgets
    QGroupBox ellipseParametersGroup("Ellipse Parameters");
    ellipseParametersGroup.setMaximumWidth(rightWidgetsMaxSize);

    QHBoxLayout ellipseAParameterLayout;
    QLabel ellipseAParameterLabel("a");
    QLineEdit ellipseAParameterEdit;
    ellipseAParameterLayout.addWidget(&ellipseAParameterLabel);
    ellipseAParameterLayout.addWidget(&ellipseAParameterEdit);
    QHBoxLayout ellipseBParameterLayout;
    QLabel ellipseBParameterLabel("b");
    QLineEdit ellipseBParameterEdit;
    ellipseBParameterLayout.addWidget(&ellipseBParameterLabel);
    ellipseBParameterLayout.addWidget(&ellipseBParameterEdit);
    QHBoxLayout ellipseCParameterLayout;
    QLabel ellipseCParameterLabel("c");
    QLineEdit ellipseCParameterEdit;
    ellipseCParameterLayout.addWidget(&ellipseCParameterLabel);
    ellipseCParameterLayout.addWidget(&ellipseCParameterEdit);
    QVBoxLayout ellipseParametersLayout;
    ellipseParametersLayout.addLayout(&ellipseAParameterLayout);
    ellipseParametersLayout.addLayout(&ellipseBParameterLayout);
    ellipseParametersLayout.addLayout(&ellipseCParameterLayout);
    ellipseParametersGroup.setLayout(&ellipseParametersLayout);

    // Adaptive rendering group of widgets
    QGroupBox adaptiveRenderingGroup("Adaptive rendering");
    adaptiveRenderingGroup.setMaximumWidth(rightWidgetsMaxSize);

    QHBoxLayout adaptiveRenderingLayout;
    QLabel adaptiveRenderingSquareSize("square size");
    QLineEdit adaptiveRenderingSquareSizeEdit;
    adaptiveRenderingLayout.addWidget(&adaptiveRenderingSquareSize);
    adaptiveRenderingLayout.addWidget(&adaptiveRenderingSquareSizeEdit);
    adaptiveRenderingGroup.setLayout(&adaptiveRenderingLayout);

    OpenGLWidget glWidget;
    leftControlsLayout.addWidget(&glWidget);

    rightControlsLayout.addWidget(&ellipseParametersGroup, 0, Qt::AlignTop);
    rightControlsLayout.addWidget(&adaptiveRenderingGroup, 0, Qt::AlignTop);

    window.show();
    return QApplication::exec();
}
