#include <QApplication>
#include <QTabWidget>
#include <QVBoxLayout>

#include "cameraFactory.hpp"
#include "geometryFactory.hpp"
#include "gl.hpp"
#include "OpenGLWidget.hpp"
#include "camera/cadCameraStrategy.hpp"
#include "camera/projectionCameraStrategy.hpp"
#include "gui/EntityPropertiesWidget.hpp"
#include "gui/GridSettingsWidget.hpp"
#include "gui/SceneHierarchyWidget.hpp"

int main(int argc, char *argv[])
{
    GLSetDefaults();
    QApplication a(argc, argv);

    QWidget window;
    window.setMinimumSize(QSize(500, 500));

    const auto layout = new QHBoxLayout(&window);
    const auto leftControlsLayout = new QVBoxLayout;
    layout->addLayout(leftControlsLayout, 1);

    const auto glWidget = new OpenGLWidget;
    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftControlsLayout->addWidget(glWidget);

    // right panel: tabbed widget
    const auto tabWidget = new QTabWidget;
    tabWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(tabWidget, 0);

    // scene tab
    const auto sceneTab = new QWidget;
    const auto sceneTabLayout = new QVBoxLayout(sceneTab);
    sceneTabLayout->setAlignment(Qt::AlignTop);
    const auto hierarchyWidget = new SceneHierarchyWidget;
    const auto entityPropertiesWidget = new EntityPropertiesWidget;
    sceneTabLayout->addWidget(hierarchyWidget);
    sceneTabLayout->addWidget(entityPropertiesWidget);
    tabWidget->addTab(sceneTab, "Scene");

    // viewport tab
    const auto viewportTab = new QWidget;
    const auto viewportTabLayout = new QVBoxLayout(viewportTab);
    viewportTabLayout->setAlignment(Qt::AlignTop);
    const auto gridSettingsWidget = new GridSettingsWidget;
    viewportTabLayout->addWidget(gridSettingsWidget);
    tabWidget->addTab(viewportTab, "Viewport");

    const GeometryFactory geometryFactory(glWidget->getScene());
    void(geometryFactory.createAxis(5.0f, {}, "Axes"));
    geometryFactory.createTorus(2.0f, 0.5f, 48, 24, cadm::vec3(0, 0, 0), "Torus");

    const CameraFactory cameraFactory(glWidget->getScene());
    const auto cameraOnSphere = cameraFactory.createCameraOnSphere(0, {});
    const auto projCameraStrategy = std::make_shared<projectionCameraStrategy>(
        cameraOnSphere,
        [&] { return glWidget->width(); },
        [&] { return glWidget->height(); });
    const auto cadCamera = cameraFactory.createCadCamera({0, 0, -10}, {}, cadm::vec3::unitY());
    const auto cadCameraStrat = std::make_shared<CadCameraStrategy>(
        cadCamera,
        [&] { return glWidget->width(); },
        [&] { return glWidget->height(); });

    // glWidget->setCameraStrategy(projCameraStrategy.get());
    glWidget->setCameraStrategy(cadCameraStrat.get());


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
        [glWidget] { glWidget->update(); });

    QObject::connect(
        gridSettingsWidget,
        &GridSettingsWidget::gridPlanesChanged,
        glWidget,
        &OpenGLWidget::setGridPlanes);

    window.installEventFilter(glWidget);
    window.show();
    return QApplication::exec();
}