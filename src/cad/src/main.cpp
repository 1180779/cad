#include <QApplication>
#include <QVBoxLayout>

#include "CameraFactory.hpp"
#include "components/TransformComponent.hpp"
#include "GeometryFactory.hpp"
#include "GlCommon.hpp"
#include "OpenGLWidget.hpp"
#include "PointRegistry.hpp"
#include "camera/CadCameraStrategy.hpp"
#include "camera/ProjectionCameraStrategy.hpp"
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
    tabWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    tabWidget->setFixedWidth(450);
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

    const auto cursor = geometryFactory.createCursor({0, 0, 0}, "Cursor");
    glWidget->getScene().setActiveCursor(cursor);

    const CameraFactory cameraFactory(glWidget->getScene());
    const auto cameraOnSphere = cameraFactory.createCameraOnSphere(20, {});
    const auto projCameraStrategy = std::make_shared<ProjectionCameraStrategy>(
        cameraOnSphere,
        [&] { return glWidget->width(); },
        [&] { return glWidget->height(); });
    const auto cadCamera = cameraFactory.createCadCamera({0, 0, -10}, {}, cadm::vec3::unitY());
    const auto cadCameraStrat = std::make_shared<CadCameraStrategy>(
        cadCamera,
        [&] { return glWidget->width(); },
        [&] { return glWidget->height(); });

    glWidget->getCameraController().addCamera("CAD", cadCameraStrat);
    glWidget->getCameraController().addCamera("Projection", projCameraStrategy);


    hierarchyWidget->setScene(&glWidget->getScene());
    hierarchyWidget->setCameraController(&glWidget->getCameraController());
    entityPropertiesWidget->setScene(&glWidget->getScene());

    QObject::connect(
        glWidget,
        &OpenGLWidget::sceneChanged,
        hierarchyWidget,
        &SceneHierarchyWidget::refresh);

    QObject::connect(
        glWidget,
        &OpenGLWidget::viewportSelectionChanged,
        hierarchyWidget,
        &SceneHierarchyWidget::syncSelectionFromScene);

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::selectionChanged,
        entityPropertiesWidget,
        [entityPropertiesWidget](const QList<Entity*> &selected)
        {
            entityPropertiesWidget->setEntity(
                selected.size() == 1
                    ? selected.first()
                    : nullptr);
        });

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::selectionChanged,
        glWidget,
        [glWidget](const QList<Entity*> &selected)
        {
            for (auto &e : glWidget->getScene().getEntities())
                e->setSelected(false);
            for (auto *e : selected)
                e->setSelected(true);
            glWidget->getScene().syncPointSelectionToRegistry();
            glWidget->update();
        });

    QObject::connect(
        entityPropertiesWidget,
        &EntityPropertiesWidget::propertyChanged,
        glWidget,
        [glWidget, hierarchyWidget]
        {
            hierarchyWidget->refresh();
            glWidget->update();
        });

    QObject::connect(
        gridSettingsWidget,
        &GridSettingsWidget::gridPlanesChanged,
        glWidget,
        &OpenGLWidget::setGridPlanes);

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::deleteEntityRequested,
        glWidget,
        [glWidget, hierarchyWidget](const Entity *e)
        {
            glWidget->getScene().removeEntity(e->getId());
            hierarchyWidget->refresh();
            glWidget->update();
        });

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::setAsCursorRequested,
        glWidget,
        [glWidget](Entity *e)
        {
            glWidget->getScene().setActiveCursor(e);
        });

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::setAsCameraRequested,
        glWidget,
        [glWidget](const EntityID id)
        {
            glWidget->getCameraController().switchTo(id, glWidget->width(), glWidget->height());
        });

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::focusCameraRequested,
        glWidget,
        [glWidget](Entity *e)
        {
            glWidget->getCameraController().lookAtEntity(e);
            glWidget->update();
        });

    auto spawnPos = [glWidget]() -> cadm::vec3
    {
        if (auto *activeCursor = glWidget->getScene().getActiveCursor())
            if (const auto t = activeCursor->getComponent<TransformComponent>())
                return t.value()->getTranslation();
        return {};
    };

    auto spawnTorus = [glWidget, spawnPos]
    {
        const GeometryFactory factory(glWidget->getScene());
        void(factory.createTorus(2.0f, 0.5f, 48, 24, spawnPos()));
        emit
        glWidget->sceneChanged();
        glWidget->update();
    };

    auto spawnCursor = [glWidget, spawnPos]
    {
        const GeometryFactory factory(glWidget->getScene());
        void(factory.createCursor(spawnPos()));
        emit
        glWidget->sceneChanged();
        glWidget->update();
    };

    auto spawnPoint = [glWidget, spawnPos]
    {
        const GeometryFactory factory(glWidget->getScene());
        void(factory.createPoint(spawnPos()));
        emit
        glWidget->sceneChanged();
        glWidget->update();
    };

    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createTorusRequested, glWidget, spawnTorus);
    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createCursorRequested, glWidget, spawnCursor);
    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createPointRequested, glWidget, spawnPoint);
    QObject::connect(glWidget, &OpenGLWidget::createTorusRequested, glWidget, spawnTorus);
    QObject::connect(glWidget, &OpenGLWidget::createCursorRequested, glWidget, spawnCursor);
    QObject::connect(glWidget, &OpenGLWidget::createPointRequested, glWidget, spawnPoint);

    QObject::connect(
        &glWidget->getCameraController(),
        &CameraController::cameraChanged,
        glWidget,
        [glWidget](const std::string &) { glWidget->update(); });

    window.installEventFilter(glWidget);
    window.show();
    return QApplication::exec();
}
