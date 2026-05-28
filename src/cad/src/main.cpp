#include <QApplication>
#include <QComboBox>
#include <QVBoxLayout>

#include "CameraFactory.hpp"
#include "components/BezierC0Component.hpp"
#include "components/bezierC2Component.hpp"
#include "components/PointComponent.hpp"
#include "components/TransformComponent.hpp"
#include "GeometryFactory.hpp"
#include "GlCommon.hpp"
#include "OpenGLWidget.hpp"
#include "camera/CadCameraStrategy.hpp"
#include "camera/BlenderCameraStrategy.hpp"
#include "gui/EntityPropertiesWidget.hpp"
#include "gui/GridSettingsWidget.hpp"
#include "gui/SceneHierarchyWidget.hpp"
#include "gui/StatusBarWidget.hpp"

int main(int argc, char *argv[]) {
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

    const auto statusBar = new StatusBarWidget;
    leftControlsLayout->addWidget(statusBar);

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

    const auto pivotLabel = new QLabel("Transform pivot:");
    const auto pivotCombo = new QComboBox;
    pivotCombo->addItem("Median point", static_cast<int>(PivotMode::medianPoint));
    pivotCombo->addItem("Active cursor", static_cast<int>(PivotMode::activeCursor));
    viewportTabLayout->addWidget(pivotLabel);
    viewportTabLayout->addWidget(pivotCombo);

    const auto coordSpaceLabel = new QLabel("Transform space:");
    const auto coordSpaceCombo = new QComboBox;
    coordSpaceCombo->addItem("World", static_cast<int>(CoordSpace::world));
    coordSpaceCombo->addItem("Local", static_cast<int>(CoordSpace::local));
    viewportTabLayout->addWidget(coordSpaceLabel);
    viewportTabLayout->addWidget(coordSpaceCombo);

    tabWidget->addTab(viewportTab, "Viewport");

    // default scene entities
    const GeometryFactory geometryFactory(glWidget->getScene());
    geometryFactory.createTorus(2.0f, 0.5f, 48, 24, cadm::vec3(0, 0, 0), "Torus");

    const auto cursor = geometryFactory.createCursor({0, 0, 0}, "Cursor");
    glWidget->getScene().setActiveCursor(cursor);

    const CameraFactory cameraFactory(glWidget->getScene());
    const auto blenderCamera = cameraFactory.createBlenderCamera(20, {});
    auto blenderCameraStrategy = std::make_unique<BlenderCameraStrategy>(
        blenderCamera,
        [&] { return glWidget->width(); },
        [&] { return glWidget->height(); }
    );
    const auto cadCamera = cameraFactory.createCadCamera({0, 0, -10}, {}, cadm::vec3::unitY());
    auto cadCameraStrat = std::make_unique<CadCameraStrategy>(
        cadCamera,
        [&] { return glWidget->width(); },
        [&] { return glWidget->height(); }
    );

    glWidget->getCameraController().addCamera("Blender", std::move(blenderCameraStrategy));
    glWidget->getCameraController().addCamera("Cad", std::move(cadCameraStrat));
    statusBar->setCameraName(QString::fromStdString(glWidget->getCameraController().getActiveName()));

    hierarchyWidget->setScene(&glWidget->getScene());
    hierarchyWidget->setCameraController(&glWidget->getCameraController());
    entityPropertiesWidget->setScene(&glWidget->getScene());

    QObject::connect(
        glWidget,
        &OpenGLWidget::sceneChanged,
        hierarchyWidget,
        &SceneHierarchyWidget::refresh
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::viewportSelectionChanged,
        hierarchyWidget,
        &SceneHierarchyWidget::syncSelectionFromScene
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::viewportSelectionChanged,
        entityPropertiesWidget,
        [glWidget, entityPropertiesWidget] {
            const auto &sel = glWidget->getScene().getSelectedEntities();
            entityPropertiesWidget->setEntity(
                sel.size() == 1
                    ? *sel.begin()
                    : nullptr
            );
        }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::selectionChanged,
        entityPropertiesWidget,
        [entityPropertiesWidget](const QList<Entity*> &selected) {
            entityPropertiesWidget->setEntity(
                selected.size() == 1
                    ? selected.first()
                    : nullptr
            );
        }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::selectionChanged,
        glWidget,
        [glWidget](const QList<Entity*> &selected) {
            glWidget->getScene().clearSelection();
            for (auto *e : selected) { glWidget->getScene().setSelected(e, true); }
            glWidget->getScene().syncPointSelectionToRegistry();
            glWidget->update();
        }
    );

    QObject::connect(
        entityPropertiesWidget,
        &EntityPropertiesWidget::propertyChanged,
        glWidget,
        [glWidget, hierarchyWidget] {
            hierarchyWidget->refresh();
            glWidget->update();
        }
    );

    QObject::connect(
        entityPropertiesWidget,
        &EntityPropertiesWidget::pointSelectionChanged,
        glWidget,
        [glWidget, hierarchyWidget, entityPropertiesWidget](const QList<Entity*> &selected) {
            glWidget->getScene().clearSelection();
            for (auto *e : selected) { glWidget->getScene().setSelected(e, true); }
            glWidget->getScene().syncPointSelectionToRegistry();
            hierarchyWidget->syncSelectionFromScene();
            entityPropertiesWidget->syncBezierSelection();
            glWidget->update();
        }
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::viewportSelectionChanged,
        entityPropertiesWidget,
        &EntityPropertiesWidget::syncBezierSelection
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::sceneChanged,
        entityPropertiesWidget,
        &EntityPropertiesWidget::refreshComponents
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::geometryChanged,
        entityPropertiesWidget,
        &EntityPropertiesWidget::refreshComponents
    );

    QObject::connect(
        gridSettingsWidget,
        &GridSettingsWidget::gridPlanesChanged,
        glWidget,
        &OpenGLWidget::setGridPlanes
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::transformModeChanged,
        statusBar,
        &StatusBarWidget::setTransformMode
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::clickToAddModeChanged,
        statusBar,
        &StatusBarWidget::setClickToAddMode
    );

    QObject::connect(
        &glWidget->getCameraController(),
        &CameraController::cameraChanged,
        statusBar,
        [statusBar](const std::string &name) { statusBar->setCameraName(QString::fromStdString(name)); }
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::viewportSelectionChanged,
        statusBar,
        [glWidget, statusBar] {
            statusBar->setSelectionCount(
                static_cast<int>(glWidget->getScene().getSelectedEntities().size())
            );
        }
    );

    QObject::connect(
        glWidget,
        &OpenGLWidget::sceneChanged,
        statusBar,
        [glWidget, statusBar] {
            if (const Entity *e = glWidget->getScene().getNewPointsTargetEntity()) {
                statusBar->setActiveNewPointsTargetName(QString::fromStdString(e->getName()));
            }
            else { statusBar->setActiveNewPointsTargetName({}); }
        }
    );

    QObject::connect(
        pivotCombo,
        &QComboBox::currentIndexChanged,
        glWidget,
        [glWidget, pivotCombo](const int index) {
            glWidget->setPivotMode(static_cast<PivotMode>(pivotCombo->itemData(index).toInt()));
        }
    );

    QObject::connect(
        coordSpaceCombo,
        &QComboBox::currentIndexChanged,
        glWidget,
        [glWidget, coordSpaceCombo](const int index) {
            glWidget->setCoordSpace(static_cast<CoordSpace>(coordSpaceCombo->itemData(index).toInt()));
        }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::deleteEntityRequested,
        glWidget,
        [glWidget, hierarchyWidget](const Entity *e) {
            glWidget->removeEntity(e->getId());
            hierarchyWidget->refresh();
            glWidget->update();
        }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::setAsCursorRequested,
        glWidget,
        [glWidget](Entity *e) { glWidget->getScene().setActiveCursor(e); }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::setAsCameraRequested,
        glWidget,
        [glWidget](const EntityID id) { glWidget->getCameraController().switchTo(id); }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::focusCameraRequested,
        glWidget,
        [glWidget](Entity *e) {
            glWidget->getCameraController().lookAtEntity(e, glWidget->getScene().getPointRegistry());
            glWidget->update();
        }
    );

    auto spawnPos = [glWidget]() -> cadm::vec3 {
        if (auto *activeCursor = glWidget->getScene().getActiveCursor()) {
            if (const auto t = activeCursor->getComponent<TransformComponent>()) { return t.value()->getTranslation(); }
        }
        return {};
    };

    auto spawnTorus = [glWidget, spawnPos] {
        const GeometryFactory factory(glWidget->getScene());
        void(factory.createTorus(2.0f, 0.5f, 48, 24, spawnPos()));
        emit
        glWidget->sceneChanged();
        glWidget->update();
    };

    auto spawnCursor = [glWidget, spawnPos] {
        const GeometryFactory factory(glWidget->getScene());
        void(factory.createCursor(spawnPos()));
        emit
        glWidget->sceneChanged();
        glWidget->update();
    };

    auto spawnPoint = [glWidget, spawnPos] {
        const GeometryFactory factory(glWidget->getScene());
        auto *point = factory.createPoint(spawnPos());

        // Auto-add to active new points target
        if (const auto *pc = point->getComponent<PointComponent>().value_or(nullptr)) {
            const Scene &sc = glWidget->getScene();
            if (Entity *newPointsTargetEntity = sc.getNewPointsTargetEntity()) {
                if (const auto bezierOpt = newPointsTargetEntity->getComponent<INewPointsTargetBase>()) {
                    bezierOpt.value()->addControlPoint(pc->m_handle);
                }
            }
        }

        emit glWidget->sceneChanged();
        glWidget->update();
    };

    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createTorusRequested, glWidget, spawnTorus);
    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createCursorRequested, glWidget, spawnCursor);
    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createPointRequested, glWidget, spawnPoint);
    QObject::connect(glWidget, &OpenGLWidget::createTorusRequested, glWidget, spawnTorus);
    QObject::connect(glWidget, &OpenGLWidget::createCursorRequested, glWidget, spawnCursor);
    QObject::connect(glWidget, &OpenGLWidget::createPointRequested, glWidget, spawnPoint);

    // Bezier C0 signals
    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::createBezierC0Requested,
        glWidget,
        [glWidget] {
            // Collect currently selected point handles
            std::vector<PointHandle> handles;
            for (const auto &e : glWidget->getScene().getEntities()) {
                if (!e->isSelected()) { continue; }
                if (const auto pc = e->getComponent<PointComponent>()) { handles.push_back(pc.value()->m_handle); }
            }
            const GeometryFactory factory(glWidget->getScene());
            void(factory.createBezierC0(handles, "BezierC0"));
            emit glWidget->sceneChanged();
            glWidget->update();
        }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::setAsNewPointsTargetEntityRequested,
        glWidget,
        [glWidget](Entity *e) {
            glWidget->getScene().setNewPointsTargetEntity(e);
            emit glWidget->sceneChanged();
        }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::addSelectedPointsToNewPointsTargetEntityRequested,
        glWidget,
        [glWidget](Entity *entity) {
            const auto newPointsTargetOpt = entity->getComponent<INewPointsTargetBase>();
            if (!newPointsTargetOpt) { return; }
            auto *pNewPointsTarget = newPointsTargetOpt.value();
            for (const auto &e : glWidget->getScene().getEntities()) {
                if (!e->isSelected()) { continue; }
                if (const auto pc = e->getComponent<PointComponent>()) {
                    pNewPointsTarget->addControlPoint(pc.value()->m_handle);
                }
            }
            emit glWidget->sceneChanged();
            glWidget->update();
        }
    );

    // Bezier C2 signals
    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::createBezierC2Requested,
        glWidget,
        [glWidget] {
            std::vector<PointHandle> handles;
            for (const auto &e : glWidget->getScene().getEntities()) {
                if (!e->isSelected()) { continue; }
                if (const auto pc = e->getComponent<PointComponent>()) { handles.push_back(pc.value()->m_handle); }
            }
            const GeometryFactory factory(glWidget->getScene());
            void(factory.createBezierC2(handles, "BezierC2"));
            emit glWidget->sceneChanged();
            glWidget->update();
        }
    );

    // QObject::connect(
    //     hierarchyWidget,
    //     &SceneHierarchyWidget::setAsActiveBezierC2Requested,
    //     glWidget,
    //     [glWidget](Entity *e)
    //     {
    //         glWidget->getScene().setActiveBezierC2(e);
    //         emit glWidget->sceneChanged();
    //     });
    //
    // QObject::connect(
    //     hierarchyWidget,
    //     &SceneHierarchyWidget::addSelectedPointsToBezierC2Requested,
    //     glWidget,
    //     [glWidget](Entity *curveEntity)
    //     {
    //         const auto bezierOpt = curveEntity->getComponent<BezierC2Component>();
    //         if (!bezierOpt) return;
    //         auto *bezier = bezierOpt.value();
    //         for (const auto &e : glWidget->getScene().getEntities())
    //         {
    //             if (!e->isSelected()) continue;
    //             if (const auto pc = e->getComponent<PointComponent>())
    //                 bezier->addControlPoint(pc.value()->m_handle);
    //         }
    //         emit glWidget->sceneChanged();
    //         glWidget->update();
    //     });

    QObject::connect(
        &glWidget->getCameraController(),
        &CameraController::cameraChanged,
        glWidget,
        [glWidget](const std::string &) { glWidget->update(); }
    );

    QApplication::instance()->installEventFilter(glWidget);
    window.show();
    return QApplication::exec();
}
