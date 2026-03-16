#include <QApplication>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDebug>
#include <QDoubleValidator>
#include <QLabel>
#include <QPushButton>

#include "gl.h"
#include "OpenGLWidget.h"

int main(int argc, char *argv[])
{
    GLSetDefaults();
    QApplication a(argc, argv);

    QWidget window;
    window.setMinimumSize(QSize(500, 500));

    constexpr int rightWidgetsMaxSize = 300;

    const auto layout = new QHBoxLayout(&window);
    const auto rightControlsLayout = new QVBoxLayout;
    rightControlsLayout->setAlignment(Qt::AlignTop);
    const auto leftControlsLayout = new QVBoxLayout;
    layout->addLayout(leftControlsLayout);
    layout->addLayout(rightControlsLayout);

    const auto glWidget = new OpenGLWidget;
    leftControlsLayout->addWidget(glWidget);

    // help text
    const auto helpText = new QLabel("Placeholder help text");
    helpText->setMaximumWidth(rightWidgetsMaxSize);
    rightControlsLayout->addWidget(helpText);

    window.installEventFilter(glWidget);
    window.show();
    return QApplication::exec();
}
