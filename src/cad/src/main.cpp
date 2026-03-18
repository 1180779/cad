#include <QApplication>
#include <QVBoxLayout>

#include "cameraFactory.hpp"
#include "gl.h"
#include "OpenGLWidget.h"
#include "gui/EntityPropertiesWidget.h"
#include "gui/SceneHierarchyWidget.h"
#include "geometryFactory.h"
#include "camera/cadCameraStrategy.hpp"

int main(int argc, char *argv[])
{
    GLSetDefaults();
    QApplication a(argc, argv);

    QWidget window;
    window.setMinimumSize(QSize(500, 500));

    constexpr int rightWidgetsMaxSize = 350;

    const auto layout = new QHBoxLayout(&window);
    const auto rightControlsLayout = new QVBoxLayout;
    rightControlsLayout->setAlignment(Qt::AlignTop);
    const auto leftControlsLayout = new QVBoxLayout;
    layout->addLayout(leftControlsLayout, 1);
    layout->addLayout(rightControlsLayout, 0);

    const auto glWidget = new OpenGLWidget;
    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftControlsLayout->addWidget(glWidget);

    const auto hierarchyWidget = new SceneHierarchyWidget;
    hierarchyWidget->setMaximumWidth(rightWidgetsMaxSize);
    hierarchyWidget->setMinimumWidth(rightWidgetsMaxSize);
    rightControlsLayout->addWidget(hierarchyWidget);

    const auto entityPropertiesWidget = new EntityPropertiesWidget;
    entityPropertiesWidget->setMaximumWidth(rightWidgetsMaxSize);
    rightControlsLayout->addWidget(entityPropertiesWidget);

    const GeometryFactory geometryFactory(glWidget->getScene());
    geometryFactory.createTorus(2.0f, 0.5f, 48, 24, cadm::vec3(0, 0, 0), "Torus");
    geometryFactory.createTorus(3.0f, 0.2f, 24, 12, cadm::vec3(5, 0, 0), "Torus 2");

    const CameraFactory cameraFactory(glWidget->getScene());
    const auto camera = cameraFactory.createArcBallCamera({0, 0, 10}, {}, cadm::vec3::unitY());
    const auto cadCameraStrategy = std::make_unique<CadCameraStrategy>(
        camera,
        [&] { return glWidget->width(); },
        [&] { return glWidget->height(); });
    glWidget->setCameraStrategy(cadCameraStrategy.get());

    hierarchyWidget->setScene(&glWidget->getScene());

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::entitySelected,
        entityPropertiesWidget,
        &EntityPropertiesWidget::setEntity);

    QObject::connect(
        entityPropertiesWidget,
        &EntityPropertiesWidget::propertyChanged,
        glWidget,
        [glWidget]() { glWidget->update(); });

    window.installEventFilter(glWidget);
    window.show();
    return QApplication::exec();
}
