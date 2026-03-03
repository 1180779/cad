#include <QApplication>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QDebug>
#include <QDoubleValidator>
#include <QLocale>
#include <functional>

#include "OpenGLWidget.h"

void addFloatParameter(QVBoxLayout* parentLayout, const QString& labelText, float initialValue, std::function<void(float)> setter)
{
    const auto layout = new QHBoxLayout;
    const auto label = new QLabel(labelText);
    const auto edit = new QLineEdit;

    const auto validator = new QDoubleValidator(edit);
    validator->setLocale(QLocale::C);
    validator->setBottom(0.0001);
    edit->setValidator(validator);
    edit->setText(QString::number(initialValue));

    QObject::connect(edit, &QLineEdit::textChanged, [setter](const QString &text) {
        bool ok;
        const float val = text.toFloat(&ok);
        if (ok) setter(val);
    });

    layout->addWidget(label);
    layout->addWidget(edit);
    parentLayout->addLayout(layout);
}

int main(int argc, char* argv[])
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

    auto glWidget = new OpenGLWidget;
    leftControlsLayout->addWidget(glWidget);

    // Ellipse Parameters group of widgets
    const auto ellipseParametersGroup = new QGroupBox("Ellipse Parameters");
    ellipseParametersGroup->setMaximumWidth(rightWidgetsMaxSize);
    const auto ellipseParametersLayout = new QVBoxLayout;
    ellipseParametersGroup->setLayout(ellipseParametersLayout);

    addFloatParameter(ellipseParametersLayout, "a", glWidget->getA(), [glWidget](float v){ glWidget->setA(v); });
    addFloatParameter(ellipseParametersLayout, "b", glWidget->getB(), [glWidget](float v){ glWidget->setB(v); });
    addFloatParameter(ellipseParametersLayout, "c", glWidget->getC(), [glWidget](float v){ glWidget->setC(v); });

    rightControlsLayout->addWidget(ellipseParametersGroup, 0, Qt::AlignTop);

    // Adaptive rendering group of widgets
    const auto adaptiveRenderingGroup = new QGroupBox("Adaptive rendering");
    adaptiveRenderingGroup->setMaximumWidth(rightWidgetsMaxSize);

    const auto adaptiveRenderingLayout = new QHBoxLayout;
    const auto adaptiveRenderingSquareSize = new QLabel("square size");
    const auto adaptiveRenderingSquareSizeEdit = new QLineEdit;
    adaptiveRenderingLayout->addWidget(adaptiveRenderingSquareSize);
    adaptiveRenderingLayout->addWidget(adaptiveRenderingSquareSizeEdit);

    adaptiveRenderingGroup->setLayout(adaptiveRenderingLayout);
    rightControlsLayout->addWidget(adaptiveRenderingGroup, 0, Qt::AlignTop);

    // Phong model parameters group of widgets
    const auto phongParametersGroup = new QGroupBox("Phong parameters");
    phongParametersGroup->setMaximumWidth(rightWidgetsMaxSize);
    const auto phongParametersLayout = new QVBoxLayout;
    addFloatParameter(phongParametersLayout, "m", glWidget->getM(), [glWidget](float v){ glWidget->setM(v); });

    phongParametersGroup->setLayout(phongParametersLayout);
    rightControlsLayout->addWidget(phongParametersGroup, 0, Qt::AlignTop);

    window.show();
    return QApplication::exec();
}
