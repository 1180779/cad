#include <QApplication>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDebug>
#include <QDoubleValidator>
#include <QLocale>
#include <functional>
#include <QLabel>
#include <QPushButton>

#include "OpenGLWidget.h"
#include <common/DoubleSlider.h>

void addFloatParameter(
    QVBoxLayout *parentLayout,
    const QString &labelText,
    const float initialValue,
    const std::function<void(float)> &setter,
    const double mappingRangeStart = 0.0,
    const double mappingRangeEnd = 1.0)
{
    const auto layout = new QHBoxLayout;
    const auto label = new QLabel(labelText);
    const auto edit = new QLineEdit;
    const auto slider = new DoubleSlider;
    slider->setMappingRange(mappingRangeStart, mappingRangeEnd);
    slider->setRange(0, 100);
    slider->setOrientation(Qt::Orientation::Horizontal);

    const auto validator = new QDoubleValidator(edit);
    validator->setLocale(QLocale::C);
    validator->setBottom(0.0001);
    edit->setValidator(validator);
    edit->setText(QString::number(initialValue));
    slider->setValue(initialValue);

    QObject::connect(
        edit,
        &QLineEdit::textEdited,
        [setter, slider](const QString &text)
        {
            bool ok;
            const float val = text.toFloat(&ok);
            if (ok)
            {
                setter(val);
                const bool oldState = slider->blockSignals(true);
                slider->setValue(val);
                slider->blockSignals(oldState);
            }
        });
    QObject::connect(
        slider,
        &DoubleSlider::doubleValueChanged,
        [setter, edit](const double val)
        {
            setter(val);
            edit->setText(QString::number(val, 'g', 3));
        });

    layout->addWidget(label);
    layout->addWidget(edit);
    parentLayout->addLayout(layout);
    parentLayout->addWidget(slider);
}

void addIntColor8BitParameter(
    QVBoxLayout *parentLayout,
    const QString &labelText,
    const int initialValue,
    const std::function<void(int)> &setter)
{
    const auto layout = new QHBoxLayout;
    const auto label = new QLabel(labelText);
    const auto edit = new QLineEdit;

    const auto validator = new QIntValidator(edit);
    validator->setLocale(QLocale::C);
    validator->setBottom(0);
    validator->setTop(255);
    edit->setValidator(validator);
    edit->setText(QString::number(initialValue));

    QObject::connect(
        edit,
        &QLineEdit::textChanged,
        [setter](const QString &text)
        {
            bool ok;
            const int val = text.toInt(&ok);
            if (ok) setter(val);
        });

    layout->addWidget(label);
    layout->addWidget(edit);
    parentLayout->addLayout(layout);
}

void addIntParameter(
    QVBoxLayout *parentLayout,
    const QString &labelText,
    const int initialValue,
    const std::function<void(int)> &setter,
    int minValue,
    int maxValue)
{
    const auto layout = new QHBoxLayout;
    const auto label = new QLabel(labelText);
    const auto edit = new QLineEdit;
    const auto slider = new QSlider;
    slider->setRange(minValue, maxValue);
    slider->setOrientation(Qt::Orientation::Horizontal);

    const auto validator = new QIntValidator(edit);
    validator->setLocale(QLocale::C);
    validator->setBottom(minValue);
    validator->setTop(maxValue);
    edit->setValidator(validator);
    edit->setText(QString::number(initialValue));
    slider->setValue(initialValue);

    QObject::connect(
        edit,
        &QLineEdit::textEdited,
        [setter, slider](const QString &text)
        {
            bool ok;
            const int val = static_cast<int>(text.toUInt(&ok));
            if (ok)
            {
                setter(val);
                const bool oldState = slider->blockSignals(true);
                slider->setValue(val);
                slider->blockSignals(oldState);
            }
        });
    QObject::connect(
        slider,
        &QSlider::valueChanged,
        [setter, edit](int val)
        {
            setter(val);
            edit->setText(QString::number(val));
        });

    layout->addWidget(label);
    layout->addWidget(edit);
    parentLayout->addLayout(layout);
    parentLayout->addWidget(slider);
}

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

    auto glWidget = new OpenGLWidget;
    leftControlsLayout->addWidget(glWidget);

    // Ellipse Parameters group of widgets
    const auto ellipseParametersGroup = new QGroupBox("Ellipse Parameters");
    ellipseParametersGroup->setMaximumWidth(rightWidgetsMaxSize);
    const auto ellipseParametersLayout = new QVBoxLayout;
    ellipseParametersGroup->setLayout(ellipseParametersLayout);

    addFloatParameter(ellipseParametersLayout, "a", glWidget->getA(), [glWidget](const float v) { glWidget->setA(v); });
    addFloatParameter(ellipseParametersLayout, "b", glWidget->getB(), [glWidget](const float v) { glWidget->setB(v); });
    addFloatParameter(ellipseParametersLayout, "c", glWidget->getC(), [glWidget](const float v) { glWidget->setC(v); });

    rightControlsLayout->addWidget(ellipseParametersGroup, 0, Qt::AlignTop);

    // Adaptive rendering group of widgets
    const auto adaptiveRenderingGroup = new QGroupBox("Adaptive rendering");
    adaptiveRenderingGroup->setMaximumWidth(rightWidgetsMaxSize);

    const auto adaptiveRenderingLayout = new QVBoxLayout;
    addIntParameter(
        adaptiveRenderingLayout,
        "square size",
        glWidget->getAdaptationSize(),
        [glWidget](const int v) { glWidget->setAdaptationSize(v); },
        1,
        16);

    adaptiveRenderingGroup->setLayout(adaptiveRenderingLayout);
    rightControlsLayout->addWidget(adaptiveRenderingGroup, 0, Qt::AlignTop);

    // Phong model parameters group of widgets
    const auto phongParametersGroup = new QGroupBox("Phong parameters");
    phongParametersGroup->setMaximumWidth(rightWidgetsMaxSize);
    const auto phongParametersLayout = new QVBoxLayout;
    addFloatParameter(
        phongParametersLayout,
        "m",
        glWidget->getM(),
        [glWidget](const float v) { glWidget->setM(v); },
        0.001,
        10);

    phongParametersGroup->setLayout(phongParametersLayout);
    rightControlsLayout->addWidget(phongParametersGroup, 0, Qt::AlignTop);

    const auto ambientColorGroup = new QGroupBox("Ambient color");
    const auto ambientColorLayout = new QVBoxLayout();

    addIntColor8BitParameter(
        ambientColorLayout,
        "r",
        glWidget->getAmbientR(),
        [glWidget](const int v) { glWidget->setAmbientR(v); });
    addIntColor8BitParameter(
        ambientColorLayout,
        "g",
        glWidget->getAmbientG(),
        [glWidget](const int v) { glWidget->setAmbientG(v); });
    addIntColor8BitParameter(
        ambientColorLayout,
        "b",
        glWidget->getAmbientB(),
        [glWidget](const int v) { glWidget->setAmbientB(v); });
    ambientColorGroup->setLayout(ambientColorLayout);

    phongParametersLayout->addWidget(ambientColorGroup);

    // reset buttons
    const auto resetScaleButton = new QPushButton("Reset Scale");
    QObject::connect(resetScaleButton, &QPushButton::clicked, [&] { glWidget->resetScale(); });
    rightControlsLayout->addWidget(resetScaleButton);

    const auto resetRotationButton = new QPushButton("Reset Rotation");
    QObject::connect(resetRotationButton, &QPushButton::clicked, [&] { glWidget->resetRotation(); });
    rightControlsLayout->addWidget(resetRotationButton);

    const auto resetTranslationButton = new QPushButton("Reset Translation");
    QObject::connect(resetTranslationButton, &QPushButton::clicked, [&] { glWidget->resetTranslation(); });
    rightControlsLayout->addWidget(resetTranslationButton);

    // help text
    const auto helpText = new QLabel(
        "Controls: <br>"
        "<b>Translation</b>: AWSD (XY) + QE (Z)<br>"
        "<b>Rotation</b>: move mouse to rotate around XY axis<br>"
        "&nbsp;&nbsp;&nbsp;&nbsp;Hold Z to rotate around Z axis<br>"
        "<b>Scale</b>: use mouse wheel<br>"
        "&nbsp;&nbsp;&nbsp;&nbsp;Hold X, Y, Z to scale only pressed axis");
    rightControlsLayout->addWidget(helpText);

    window.installEventFilter(glWidget);
    window.show();
    return QApplication::exec();
}