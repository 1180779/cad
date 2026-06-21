#include <QApplication>
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
#include "gui/Theme.hpp"
#include "gui/ScenePanelWidget.hpp"
#include "gui/StatusBarWidget.hpp"
#include "gui/ToolPanelBar.hpp"
#include "gui/ViewportPanelWidget.hpp"

/// @brief Width of the app-colored separator strips between viewport, tool window and status line
constexpr int gc_kSeparatorStripWidth = 5;

/// @brief Border width around the frameless window reserved for native resize handling
constexpr int gc_kResizeMargin = 6;

namespace {
    /// @brief Applies the IntelliJ-style menu theming: top-level items gray on hover,
    /// dropdowns painted in the app background with a blue + white-text highlight, and
    /// muted (not bland-gray) disabled entries derived from the app background
    void styleMenuBar(CadMenuBar *menuBar) {
        // the selected-item color defers to the live palette (palette(highlight)) so it
        // tracks the app accent and focus state; the surrounding tints are custom theme values
        menuBar->setStyleSheet(
            QStringLiteral(
                "QMenuBar { background: transparent; }"
                "QMenuBar::item { padding: 4px 8px; background: transparent; border-radius: %5px; }"
                "QMenuBar::item:selected { background: %1; }"
                "QMenuBar::item:pressed { background: %1; }"
                "QMenu { background-color: %2; border: 1px solid %3; padding: 4px; }"
                "QMenu::item { padding: 4px 24px; border-radius: %5px; }"
                "QMenu::item:selected { background-color: palette(highlight); color: palette(highlighted-text); }"
                "QMenu::item:disabled { color: %4; }"
                "QMenu::separator { height: 1px; background: %3; margin: 4px 8px; }"
            )
            .arg(
                theme::g_menuHover.name(),
                theme::gc_appBackground.name(),
                theme::g_menuBorder.name(),
                theme::g_menuDisabled.name()
            )
            .arg(theme::gc_itemRadius)
        );
    }

    void styleToolPanels(const std::initializer_list<QWidget*> panels, const QColor &base) {
        const QString qss =
            QStringLiteral("#toolPanel { background-color: %1; border-radius: %2px; }")
            .arg(base.name())
            .arg(theme::gc_cardRadius);
        for (QWidget *panel : panels) {
            panel->setObjectName("toolPanel");
            panel->setAttribute(Qt::WA_StyledBackground, true);
            panel->setStyleSheet(qss);
        }
    }

    /// @brief Wires the panel bar and Tools-menu toggles to the stacked tool windows
    void wirePanelToggles(
        const ToolPanelBar *panelBar,
        QStackedWidget *panelStack,
        const QAction *sceneAction,
        const QAction *viewportAction,
        QToolButton *sceneBtn,
        QToolButton *viewportBtn
    ) {
        // panel bar -> show/hide stack
        QObject::connect(
            panelBar,
            &ToolPanelBar::panelRequested,
            panelStack,
            [panelStack](const int index) {
                panelStack->setCurrentIndex(index);
                panelStack->show();
                panelStack->currentWidget()->setFocus(Qt::OtherFocusReason);
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
    }

    /// @brief Wires the Edit-menu Undo/Redo actions to the command stack and keeps their
    /// enabled state in sync when the menu opens
    void wireUndoRedo(CadMenuBar *menuBar, OpenGlWidget *glWidget) {
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
        const auto refreshEnabled = [glWidget, menuBar] {
            menuBar->setUndoEnabled(glWidget->getCommandStack().canUndo());
            menuBar->setRedoEnabled(glWidget->getCommandStack().canRedo());
        };
        QObject::connect(glWidget, &OpenGlWidget::sceneChanged, menuBar, refreshEnabled);
        refreshEnabled();
    }

    /// @brief Keeps viewport, hierarchy and entity-properties selection/state in sync
    void wireSelectionSync(
        OpenGlWidget *glWidget,
        SceneHierarchyWidget *hierarchyWidget,
        EntityPropertiesWidget *entityPropertiesWidget
    ) {
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
    }

    /// @brief Wires the status line to viewport transform/selection/target/camera state
    void wireStatusBar(OpenGlWidget *glWidget, StatusBarWidget *statusBar) {
        QObject::connect(glWidget, &OpenGlWidget::transformModeChanged, statusBar, &StatusBarWidget::setTransformMode);
        QObject::connect(
            glWidget,
            &OpenGlWidget::clickToAddModeChanged,
            statusBar,
            &StatusBarWidget::setClickToAddMode
        );

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
    }

    /// @brief Wires the viewport-panel controls (grid, pivot, coord space) and camera repaint
    void wireViewportControls(OpenGlWidget *glWidget, const ViewportPanelWidget *viewportPanel) {
        QObject::connect(
            viewportPanel->gridSettingsWidget(),
            &GridSettingsWidget::gridPlanesChanged,
            glWidget,
            &OpenGlWidget::setGridPlanes
        );
        glWidget->setGridPlanes(viewportPanel->gridSettingsWidget()->getGridPlanes());

        auto *pivotCombo = viewportPanel->pivotCombo();
        QObject::connect(
            pivotCombo,
            &QComboBox::currentIndexChanged,
            glWidget,
            [glWidget, pivotCombo](const int index) {
                glWidget->setPivotMode(static_cast<PivotMode>(pivotCombo->itemData(index).toInt()));
            }
        );

        auto *coordSpaceCombo = viewportPanel->coordSpaceCombo();
        QObject::connect(
            coordSpaceCombo,
            &QComboBox::currentIndexChanged,
            glWidget,
            [glWidget, coordSpaceCombo](const int index) {
                glWidget->setCoordSpace(static_cast<CoordSpace>(coordSpaceCombo->itemData(index).toInt()));
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
    }

    /// @brief Wires hierarchy context actions: delete, set-as-cursor/camera, focus camera
    void wireHierarchyActions(OpenGlWidget *glWidget, SceneHierarchyWidget *hierarchyWidget) {
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
    }

    /// @brief Wires entity-creation requests (torus/cursor/point + Bezier C0/C2) from both the
    /// hierarchy and the viewport into undoable command pushes
    void wireEntityCreation(OpenGlWidget *glWidget, const SceneHierarchyWidget *hierarchyWidget) {
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
    }
}

int main(int argc, char *argv[]) {
    glSetDefaults();
    [[maybe_unused]] QApplication a(argc, argv);

    // seed the theme onto the app-wide palette: every widget (and palette(...) QSS
    // reference) inherits the app background, white card base and accent from here
    QApplication::setPalette(theme::applyTheme(QApplication::palette()));

    QWidget window;
    window.setWindowFlag(Qt::FramelessWindowHint);
    window.setMinimumSize(QSize(500, 500));

    // window paints its (now theme-seeded) QPalette::Window background
    window.setAutoFillBackground(true);

    // outer column: custom title bar on top, app content below. The margin reserves
    // a border that the frameless-resize filter uses for native edge resizing
    // ReSharper disable once CppDFAMemoryLeak
    const auto outerLayout = new QVBoxLayout(&window);
    // spacing matches the border so the title bar is framed symmetrically
    outerLayout->setSpacing(gc_kResizeMargin);
    outerLayout->setContentsMargins(gc_kResizeMargin, gc_kResizeMargin, gc_kResizeMargin, gc_kResizeMargin);

    // custom title bar embedding the menu bar + window controls (frameless window)
    const auto menuBar = new CadMenuBar;
    styleMenuBar(menuBar);
    const auto titleBar = new CadTitleBar(menuBar);
    outerLayout->addWidget(titleBar);

    // ReSharper disable once CppDFAMemoryLeak
    const auto content = new QWidget;
    // explicit cursor so the window's border resize cursor never bleeds into the
    // content area through inheritance (child widgets still override as needed)
    content->setCursor(Qt::ArrowCursor);
    // ReSharper disable once CppDFAMemoryLeak
    const auto rootLayout = new QHBoxLayout(content);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(content, 1);

    // left side: GL widget + status bar in a container
    // ReSharper disable once CppDFAMemoryLeak
    const auto leftContainer = new QWidget;
    // ReSharper disable once CppDFAMemoryLeak
    const auto leftLayout = new QVBoxLayout(leftContainer);
    // small app-colored strip between the viewport and the vim-style status line
    leftLayout->setSpacing(gc_kSeparatorStripWidth);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    const auto glWidget = new OpenGlWidget;
    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    glWidget->setMinimumWidth(40);
    leftLayout->addWidget(glWidget);

    menuBar->applyShortcuts(glWidget->getInputMap());

    const auto statusBar = new StatusBarWidget;
    statusBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    leftLayout->addWidget(statusBar);

    const auto scenePanel = new ScenePanelWidget;
    const auto viewportPanel = new ViewportPanelWidget;

    scenePanel->setFocusPolicy(Qt::ClickFocus);
    viewportPanel->setFocusPolicy(Qt::ClickFocus);

    // ReSharper disable once CppDFAMemoryLeak
    const auto panelStack = new QStackedWidget;
    panelStack->addWidget(scenePanel);
    panelStack->addWidget(viewportPanel);
    panelStack->setMinimumWidth(200);

    styleToolPanels({scenePanel, viewportPanel}, window.palette().color(QPalette::Base));

    // ReSharper disable once CppDFAMemoryLeak
    const auto splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    // app-colored strip separating the viewport from the open tool window
    splitter->setHandleWidth(gc_kSeparatorStripWidth);
    splitter->setStyleSheet(
        QStringLiteral("QSplitter::handle { background-color: %1; }").arg(theme::gc_appBackground.name())
    );
    splitter->addWidget(leftContainer); // index 0: stretches
    splitter->addWidget(panelStack); // index 1: resizable, collapsible
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

    wirePanelToggles(panelBar, panelStack, sceneAction, viewportAction, sceneBtn, viewportBtn);

    // intelliJ-style active-tab accent: the open tab paints blue only while the tool panel
    // holds focus, falling back to the hover gray otherwise. Track app focus and report
    // whether it currently lives inside the panel stack
    QObject::connect(
        qApp,
        &QApplication::focusChanged,
        panelBar,
        [panelBar, panelStack](QWidget *, const QWidget *now) {
            panelBar->setPanelFocused(now != nullptr && panelStack->isAncestorOf(now));
        }
    );

    auto *hierarchyWidget = scenePanel->hierarchyWidget();
    auto *entityPropertiesWidget = scenePanel->entityPropertiesWidget();

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

    hierarchyWidget->setScene(&glWidget->getScene());
    hierarchyWidget->setCommandStack(&glWidget->getCommandStack());
    hierarchyWidget->setCameraController(&glWidget->getCameraController());
    entityPropertiesWidget->setScene(&glWidget->getScene());
    entityPropertiesWidget->setCommandStack(&glWidget->getCommandStack());

    wireUndoRedo(menuBar, glWidget);
    wireSelectionSync(glWidget, hierarchyWidget, entityPropertiesWidget);
    wireStatusBar(glWidget, statusBar);
    wireViewportControls(glWidget, viewportPanel);
    wireHierarchyActions(glWidget, hierarchyWidget);
    wireEntityCreation(glWidget, hierarchyWidget);

    QApplication::instance()->installEventFilter(glWidget);
    enableFramelessResize(&window, gc_kResizeMargin);
    window.show();
    return QApplication::exec();
}
