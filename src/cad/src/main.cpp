#include <QApplication>
#include <QPalette>
#include <QSplitter>
#include <QStackedWidget>

#include "CameraFactory.hpp"
#include "components/BezierC0Component.hpp"
#include "components/PointComponent.hpp"
#include "components/TransformComponent.hpp"
#include "GeometryFactory.hpp"
#include "GlCommon.hpp"
#include "OpenGLWidget.hpp"
#include "commands/Commands.hpp"
#include "components/INewPointsTargetComponent.hpp"
#include "camera/CadCameraStrategy.hpp"
#include "camera/BlenderCameraStrategy.hpp"
#include "gui/CadMenuBar.hpp"
#include "gui/CadTitleBar.hpp"
#include "gui/ScenePanelWidget.hpp"
#include "gui/StatusBarWidget.hpp"
#include "gui/ToolPanelBar.hpp"
#include "gui/ViewportPanelWidget.hpp"

/// @brief Width of the app-colored separator strips between viewport, tool window and status line
constexpr int kSeparatorStripWidth = 5;

/// @brief Soft off-white app background (IntelliJ-like), distinct from the white tool-window cards
const QColor kAppBackground(0xF2, 0xF3, 0xF5);

/// @brief Border width around the frameless window reserved for native resize handling
constexpr int kResizeMargin = 6;

int main(int argc, char *argv[]) {
    glSetDefaults();
    [[maybe_unused]] QApplication a(argc, argv);

    QWidget window;
    window.setWindowFlag(Qt::FramelessWindowHint);
    window.setMinimumSize(QSize(500, 500));

    // soft off-white app background; tool-window cards and separator strips read from this
    QPalette windowPalette = window.palette();
    windowPalette.setColor(QPalette::Window, kAppBackground);
    window.setPalette(windowPalette);
    window.setAutoFillBackground(true);

    // outer column: custom title bar on top, app content below. The margin reserves
    // a border that the frameless-resize filter uses for native edge resizing
    const auto outerLayout = new QVBoxLayout(&window);
    // spacing matches the border so the title bar is framed symmetrically
    outerLayout->setSpacing(kResizeMargin);
    outerLayout->setContentsMargins(kResizeMargin, kResizeMargin, kResizeMargin, kResizeMargin);

    // custom title bar embedding the menu bar + window controls (frameless window)
    const auto menuBar = new CadMenuBar;
    // intelliJ-style menus: top-level items grey on hover, dropdowns painted in the
    // app background with a blue + white-text highlight, and muted (not bland-grey)
    // disabled entries derived from the app background

    const QColor hover = kAppBackground.darker(112); // subtle grey wash on hover
    const QColor highlight = window.palette().color(QPalette::Highlight);
    const QColor disabled = kAppBackground.darker(140); // muted, tinted from app bg
    const QColor border = kAppBackground.darker(125);
    menuBar->setStyleSheet(
        QStringLiteral(
            "QMenuBar { background: transparent; }"
            "QMenuBar::item { padding: 4px 8px; background: transparent; border-radius: 3px; }"
            "QMenuBar::item:selected { background: %1; }"
            "QMenuBar::item:pressed { background: %1; }"
            "QMenu { background-color: %2; border: 1px solid %5; padding: 4px; }"
            "QMenu::item { padding: 4px 24px; border-radius: 3px; }"
            "QMenu::item:selected { background-color: %3; color: white; }"
            "QMenu::item:disabled { color: %4; }"
            "QMenu::separator { height: 1px; background: %5; margin: 4px 8px; }"
        )
        .arg(hover.name(), kAppBackground.name(), highlight.name(), disabled.name(), border.name())
    );
    const auto titleBar = new CadTitleBar(menuBar);
    outerLayout->addWidget(titleBar);

    const auto content = new QWidget;
    // explicit cursor so the window's border resize cursor never bleeds into the
    // content area through inheritance (child widgets still override as needed)
    content->setCursor(Qt::ArrowCursor);
    const auto rootLayout = new QHBoxLayout(content);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(content, 1);

    // left side: GL widget + status bar in a container
    const auto leftContainer = new QWidget;
    const auto leftLayout = new QVBoxLayout(leftContainer);
    // small app-colored strip between the viewport and the vim-style status line
    leftLayout->setSpacing(kSeparatorStripWidth);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    const auto glWidget = new OpenGlWidget;
    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftLayout->addWidget(glWidget);

    const auto statusBar = new StatusBarWidget;
    leftLayout->addWidget(statusBar);

    const auto scenePanel = new ScenePanelWidget;
    const auto viewportPanel = new ViewportPanelWidget;

    const auto panelStack = new QStackedWidget;
    panelStack->addWidget(scenePanel); // index 0
    panelStack->addWidget(viewportPanel); // index 1
    panelStack->setMinimumWidth(200);

    // rounded, white tool-window cards floating on the app background. The
    // QStackedWidget stays transparent so the panels' rounded corners reveal the
    // app color behind them; each panel paints its own rounded base-colored card
    const QColor panelBg = window.palette().color(QPalette::Base);
    const QString panelQss =
        QStringLiteral("#toolPanel { background-color: %1; border-radius: 6px; }").arg(panelBg.name());
    for (QWidget *panel : {
             static_cast<QWidget*>(scenePanel),
             static_cast<QWidget*>(viewportPanel)
         }) {
        panel->setObjectName("toolPanel");
        // custom QWidget subclasses ignore stylesheet background unless styled-background is on
        panel->setAttribute(Qt::WA_StyledBackground, true);
        panel->setStyleSheet(panelQss);
    }

    // splitter owns left + panelStack; panelBar sits outside so hiding panelStack
    // collapses cleanly without leaving an empty rightContainer
    const auto splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    // app-colored strip separating the viewport from the open tool window
    splitter->setHandleWidth(kSeparatorStripWidth);
    splitter->setStyleSheet(
        QStringLiteral("QSplitter::handle { background-color: %1; }").arg(kAppBackground.name())
    );
    splitter->addWidget(leftContainer); // index 0 — stretches
    splitter->addWidget(panelStack); // index 1 — resizable, collapsible
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({10000, 450});
    rootLayout->addWidget(splitter, 1);

    const auto panelBar = new ToolPanelBar;
    rootLayout->addWidget(panelBar, 0);

    const auto *sceneAction = menuBar->addToolPanelAction("Scene");
    const auto *viewportAction = menuBar->addToolPanelAction("Viewport");

    auto *sceneBtn = panelBar->addPanel("Scene");
    auto *viewportBtn = panelBar->addPanel("Viewport");

    // open Scene panel by default
    panelBar->openPanel(0);
    panelStack->setCurrentIndex(0);
    panelStack->show();

    // panel bar → show/hide stack
    QObject::connect(
        panelBar,
        &ToolPanelBar::panelRequested,
        panelStack,
        [panelStack](const int index) {
            panelStack->setCurrentIndex(index);
            panelStack->show();
        }
    );
    QObject::connect(
        panelBar,
        &ToolPanelBar::panelClosed,
        panelStack,
        [panelStack] {
            panelStack->hide();
        }
    );

    // tools menu <-> panel buttons
    QObject::connect(sceneAction, &QAction::toggled, sceneBtn, &QToolButton::setVisible);
    QObject::connect(viewportAction, &QAction::toggled, viewportBtn, &QToolButton::setVisible);
    QObject::connect(
        sceneAction,
        &QAction::toggled,
        sceneBtn,
        [sceneBtn, panelStack](const bool visible) {
            if (!visible && sceneBtn->isChecked()) {
                sceneBtn->setChecked(false);
                panelStack->hide();
            }
        }
    );
    QObject::connect(
        viewportAction,
        &QAction::toggled,
        viewportBtn,
        [viewportBtn, panelStack](const bool visible) {
            if (!visible && viewportBtn->isChecked()) {
                viewportBtn->setChecked(false);
                panelStack->hide();
            }
        }
    );

    // convenience aliases
    auto *hierarchyWidget = scenePanel->hierarchyWidget();
    auto *entityPropertiesWidget = scenePanel->entityPropertiesWidget();
    const auto *gridSettingsWidget = viewportPanel->gridSettingsWidget();
    auto *pivotCombo = viewportPanel->pivotCombo();
    auto *coordSpaceCombo = viewportPanel->coordSpaceCombo();

    // default scene entities
    const GeometryFactory geometryFactory(glWidget->getScene());
    geometryFactory.createTorus(2.0f, 0.5f, 48, 24, cadm::Vec3(0, 0, 0), "Torus");

    const auto cursor = geometryFactory.createCursor({0, 0, 0}, "Cursor");
    glWidget->getScene().setActiveCursor(cursor);

    const CameraFactory cameraFactory(glWidget->getScene());
    const auto blenderCamera = cameraFactory.createBlenderCamera(20, {});
    auto blenderCameraStrategy = std::make_unique<BlenderCameraStrategy>(
        blenderCamera,
        [&] {
            return glWidget->width();
        },
        [&] {
            return glWidget->height();
        }
    );
    const auto cadCamera = cameraFactory.createCadCamera({0, 0, -10}, {}, cadm::Vec3::unitY());
    auto cadCameraStrat = std::make_unique<CadCameraStrategy>(
        cadCamera,
        [&] {
            return glWidget->width();
        },
        [&] {
            return glWidget->height();
        }
    );

    glWidget->getCameraController().addCamera("Blender", std::move(blenderCameraStrategy));
    glWidget->getCameraController().addCamera("Cad", std::move(cadCameraStrat));
    statusBar->setCameraName(QString::fromStdString(glWidget->getCameraController().getActiveName()));

    // menu bar undo/redo
    QObject::connect(
        menuBar,
        &CadMenuBar::undoRequested,
        glWidget,
        [glWidget] {
            glWidget->getCommandStack().undo();
        }
    );
    QObject::connect(
        menuBar,
        &CadMenuBar::redoRequested,
        glWidget,
        [glWidget] {
            glWidget->getCommandStack().redo();
        }
    );
    // update enabled state when Edit menu opens
    QObject::connect(
        menuBar,
        &CadMenuBar::editMenuAboutToShow,
        glWidget,
        [glWidget, menuBar] {
            menuBar->setUndoEnabled(glWidget->getCommandStack().canUndo());
            menuBar->setRedoEnabled(glWidget->getCommandStack().canRedo());
        }
    );

    hierarchyWidget->setScene(&glWidget->getScene());
    hierarchyWidget->setCommandStack(&glWidget->getCommandStack());
    hierarchyWidget->setCameraController(&glWidget->getCameraController());
    entityPropertiesWidget->setScene(&glWidget->getScene());
    entityPropertiesWidget->setCommandStack(&glWidget->getCommandStack());

    QObject::connect(glWidget, &OpenGlWidget::sceneChanged, hierarchyWidget, &SceneHierarchyWidget::refresh);

    QObject::connect(
        glWidget,
        &OpenGlWidget::viewportSelectionChanged,
        hierarchyWidget,
        &SceneHierarchyWidget::syncSelectionFromScene
    );

    QObject::connect(
        glWidget,
        &OpenGlWidget::viewportSelectionChanged,
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
            for (auto *e : selected) {
                glWidget->getScene().setSelected(e, true);
            }
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
            for (auto *e : selected) {
                glWidget->getScene().setSelected(e, true);
            }
            glWidget->getScene().syncPointSelectionToRegistry();
            hierarchyWidget->syncSelectionFromScene();
            entityPropertiesWidget->syncBezierSelection();
            glWidget->update();
        }
    );

    QObject::connect(
        glWidget,
        &OpenGlWidget::viewportSelectionChanged,
        entityPropertiesWidget,
        &EntityPropertiesWidget::syncBezierSelection
    );

    QObject::connect(
        glWidget,
        &OpenGlWidget::sceneChanged,
        entityPropertiesWidget,
        &EntityPropertiesWidget::refreshComponents
    );

    QObject::connect(
        glWidget,
        &OpenGlWidget::geometryChanged,
        entityPropertiesWidget,
        &EntityPropertiesWidget::refreshComponents
    );

    QObject::connect(
        gridSettingsWidget,
        &GridSettingsWidget::gridPlanesChanged,
        glWidget,
        &OpenGlWidget::setGridPlanes
    );

    QObject::connect(glWidget, &OpenGlWidget::transformModeChanged, statusBar, &StatusBarWidget::setTransformMode);
    QObject::connect(glWidget, &OpenGlWidget::clickToAddModeChanged, statusBar, &StatusBarWidget::setClickToAddMode);

    QObject::connect(
        &glWidget->getCameraController(),
        &CameraController::cameraChanged,
        statusBar,
        [statusBar](const std::string &name) {
            statusBar->setCameraName(QString::fromStdString(name));
        }
    );

    QObject::connect(
        glWidget,
        &OpenGlWidget::viewportSelectionChanged,
        statusBar,
        [glWidget, statusBar] {
            statusBar->setSelectionCount(
                static_cast<int>(glWidget->getScene().getSelectedEntities().size())
            );
        }
    );

    QObject::connect(
        glWidget,
        &OpenGlWidget::sceneChanged,
        statusBar,
        [glWidget, statusBar] {
            if (const Entity *e = glWidget->getScene().getNewPointsTargetEntity()) {
                statusBar->setActiveNewPointsTargetName(QString::fromStdString(e->getName()));
            }
            else {
                statusBar->setActiveNewPointsTargetName({});
            }
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
        [glWidget](Entity *e) {
            glWidget->getScene().setActiveCursor(e);
        }
    );

    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::setAsCameraRequested,
        glWidget,
        [glWidget](const EntityId id) {
            glWidget->getCameraController().switchTo(id);
        }
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

    // ReSharper disable once CppDFAUnreachableFunctionCall
    auto spawnPos = [glWidget]() -> cadm::Vec3 {
        if (auto *activeCursor = glWidget->getScene().getActiveCursor()) {
            if (const auto t = activeCursor->getComponent<TransformComponent>()) {
                return t.value()->getTranslation();
            }
        }
        return {};
    };

    auto spawnTorus = [glWidget, spawnPos] {
        glWidget->getCommandStack().push(
            std::make_unique<CreateEntityCommand>(
                glWidget->getScene(),
                [pos = spawnPos()](Scene &s) {
                    return GeometryFactory(s).createTorus(2.0f, 0.5f, 48, 24, pos);
                }
            )
        );
    };

    auto spawnCursor = [glWidget, spawnPos] {
        glWidget->getCommandStack().push(
            std::make_unique<CreateEntityCommand>(
                glWidget->getScene(),
                [pos = spawnPos()](Scene &s) {
                    return GeometryFactory(s).createCursor(pos);
                }
            )
        );
    };

    auto spawnPoint = [glWidget, spawnPos] {
        Scene &sc = glWidget->getScene();
        PointHandle createdHandle = InvalidPointHandle;
        glWidget->getCommandStack().push(
            std::make_unique<CreateEntityCommand>(
                sc,
                [pos = spawnPos(), &createdHandle](Scene &s) {
                    Entity *point = GeometryFactory(s).createPoint(pos);
                    if (const auto pc = point->getComponent<PointComponent>()) {
                        createdHandle = pc.value()->m_handle;
                    }
                    return point;
                }
            )
        );

        // auto-add to active new points target as a separate undoable step
        if (const Entity *target = sc.getNewPointsTargetEntity();
            target && createdHandle != InvalidPointHandle && target->hasComponent<INewPointsTargetBase>()) {
            glWidget->getCommandStack().push(
                std::make_unique<AddControlPointCommand>(sc, target->getId(), createdHandle)
            );
        }
    };

    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createTorusRequested, glWidget, spawnTorus);
    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createCursorRequested, glWidget, spawnCursor);
    QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createPointRequested, glWidget, spawnPoint);
    QObject::connect(glWidget, &OpenGlWidget::createTorusRequested, glWidget, spawnTorus);
    QObject::connect(glWidget, &OpenGlWidget::createCursorRequested, glWidget, spawnCursor);
    QObject::connect(glWidget, &OpenGlWidget::createPointRequested, glWidget, spawnPoint);

    // Bezier C0 signals
    QObject::connect(
        hierarchyWidget,
        &SceneHierarchyWidget::createBezierC0Requested,
        glWidget,
        [glWidget] {
            std::vector<PointHandle> handles;
            for (const auto &e : glWidget->getScene().getEntities()) {
                if (!e->isSelected()) {
                    continue;
                }
                if (const auto pc = e->getComponent<PointComponent>()) {
                    handles.push_back(pc.value()->m_handle);
                }
            }
            glWidget->getCommandStack().push(
                std::make_unique<CreateEntityCommand>(
                    glWidget->getScene(),
                    [handles](Scene &s) {
                        return GeometryFactory(s).createBezierC0(handles, "BezierC0");
                    }
                )
            );
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
        [glWidget](const Entity *entity) {
            if (!entity->hasComponent<INewPointsTargetBase>()) {
                return;
            }
            Scene &sc = glWidget->getScene();
            const EntityId curveId = entity->getId();
            for (const auto &e : sc.getEntities()) {
                if (!e->isSelected()) {
                    continue;
                }
                if (const auto pc = e->getComponent<PointComponent>()) {
                    glWidget->getCommandStack().push(
                        std::make_unique<AddControlPointCommand>(sc, curveId, pc.value()->m_handle)
                    );
                }
            }
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
                if (!e->isSelected()) {
                    continue;
                }
                if (const auto pc = e->getComponent<PointComponent>()) {
                    handles.push_back(pc.value()->m_handle);
                }
            }
            glWidget->getCommandStack().push(
                std::make_unique<CreateEntityCommand>(
                    glWidget->getScene(),
                    [handles](Scene &s) {
                        return GeometryFactory(s).createBezierC2(handles, "BezierC2");
                    }
                )
            );
        }
    );

    QObject::connect(
        &glWidget->getCameraController(),
        &CameraController::cameraChanged,
        glWidget,
        [glWidget](const std::string &) {
            glWidget->update();
        }
    );

    QApplication::instance()->installEventFilter(glWidget);
    enableFramelessResize(&window, kResizeMargin);
    window.show();
    return QApplication::exec();
}
