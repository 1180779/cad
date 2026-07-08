#include <tuple>
#include <vector>

#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QPainter>
#include <QSplitter>

#include "factory/CameraFactory.hpp"
#include "components/geometry/BezierC0Component.hpp"
#include "components/PointComponent.hpp"
#include "components/TransformComponent.hpp"
#include "factory/GeometryFactory.hpp"
#include "GlCommon.hpp"
#include "OpenGLWidget.hpp"
#include "PersistentEntities.hxx"
#include "commands/Commands.hpp"
#include "components/INewPointsTargetComponent.hpp"
#include "camera/CadCameraStrategy.hpp"
#include "camera/BlenderCameraStrategy.hpp"
#include "gui/CadMenuBar.hpp"
#include "gui/components/geometry/PatchCreatorDialog.hxx"
#include "gui/CadTitleBar.hpp"
#include "gui/Theme.hpp"
#include "gui/toolbars/properties/PropertiesPanelWidget.hxx"
#include "gui/toolbars/scene/ScenePanelWidget.hpp"
#include "gui/StatusBarWidget.hpp"
#include "gui/toolbars/SubdividedPanelBar.hxx"
#include "gui/toolbars/ToolPanelBar.hpp"
#include "gui/toolbars/viewport/ViewportPanelWidget.hpp"
#include "serialization/Serialization.hxx"

/// @brief Width of the app-colored separator strips between viewport, tool
/// window and status line
constexpr int gc_separatorStripWidth = 5;

/// @brief Border width around the frameless window reserved for native resize
/// handling
constexpr int gc_resizeMargin = 6;

namespace {
    /// @brief Marks tool-window panels to the app-wide #toolPanel rule
    void prepareToolPanels(const std::initializer_list<QWidget*> panels) {
        for (QWidget *panel : panels) {
            panel->setObjectName("toolPanel");
            panel->setAttribute(Qt::WA_StyledBackground, true);
        }
    }

    /// @brief One Tools-menu entry <-> tab-bar-button <-> dock-panel-index
    /// triple
    struct ToolPanelToggle {
        const QAction *action;
        QToolButton *button;
        int index;
    };

    /// @brief Wires the panel bar and Tools-menu toggles to the dockable panel
    /// stack
    void wirePanelToggles(
        ToolPanelBar *panelBar,
        SubdividedPanelBar *panelStack,
        const std::vector<ToolPanelToggle> &toggles
    ) {
        using namespace aliases;
        auto showPanel = [panelBar, panelStack](const int index) {
            panelStack->showPanel(index, panelBar->groupOf(index));
        };
        // ReSharper disable once CppDFAUnreachableFunctionCall
        auto uncheckButton = [panelBar](const int index) {
            if (auto *btn = panelBar->buttonAt(index)) {
                btn->setChecked(false);
            }
        };
        auto closeOnHide = [uncheckButton, panelStack](const int index) {
            return [uncheckButton, panelStack, index](const bool visible) {
                if (!visible) {
                    uncheckButton(index);
                    panelStack->hidePanel(index);
                }
            };
        };

        QObject::connect(panelBar, &ToolPB::panelRequested, panelStack, showPanel);
        QObject::connect(panelBar, &ToolPB::panelClosed, panelStack, &SubdividedPanelBar::hidePanel);
        QObject::connect(panelStack, &SubdividedPanelBar::panelClosedByUser, panelBar, uncheckButton);

        for (const auto &[action, button, index] : toggles) {
            QObject::connect(action, &QAction::toggled, button, &QToolButton::setVisible);
            QObject::connect(action, &QAction::toggled, button, closeOnHide(index));
        }
    }

    /// @brief Creates the default cursor + default cameras for the scene
    void setupDefaultCamerasAndCursor(OpenGlWidget *glW) {
        Scene &scene = glW->getScene();
        const GeometryFactory geometryFactory(scene);
        scene.setActiveCursor(geometryFactory.createCursor({0, 0, 0}, "Cursor"));

        const auto widthOf = [glW] {
            return glW->width();
        };
        const auto heightOf = [glW] {
            return glW->height();
        };

        const CameraFactory cameraFactory(scene);
        const auto blenderCamera = cameraFactory.createBlenderCamera(20, {});
        auto blenderCameraStrategy = std::make_unique<BlenderCameraStrategy>(blenderCamera, widthOf, heightOf);
        const auto cadCamera = cameraFactory.createCadCamera({0, 0, -10}, {}, cadm::Vec3::unitY());
        auto cadCameraStrat = std::make_unique<CadCameraStrategy>(cadCamera, widthOf, heightOf);

        glW->getCameraController().addCamera("Blender", std::move(blenderCameraStrategy));
        glW->getCameraController().addCamera("Cad", std::move(cadCameraStrat));
        glW->getCameraController().getActiveStrategy()->syncAspectRatio();
    }

    /// @brief Wires New/Save/Save As/Open to a native file dialog and JSON
    /// (de)serialization. The current file path is process-lifetime state
    void wireFileMenu(const CadMenuBar *menuBar, OpenGlWidget *glW, EntityPropertiesWidget *entityPropsWidget) {
        static QString currentFilePath;

        const auto writeToPath = [glW](const QString &path) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) {
                QMessageBox::warning(glW, "Save Failed", "Could not open file for writing:\n" + path);
                return false;
            }
            file.write(serialization::toJson(glW->getScene()).toJson());
            return true;
        };
        const auto saveAs = [glW, writeToPath] {
            static const QString jsonFilter = "Scene Files (*.json)";
            QString selectedFilter = jsonFilter;
            QString path = QFileDialog::getSaveFileName(
                glW,
                "Save Scene",
                {},
                jsonFilter + ";;All Files (*)",
                &selectedFilter
            );
            if (path.isEmpty()) {
                return;
            }
            if (selectedFilter == jsonFilter && !path.endsWith(".json", Qt::CaseInsensitive)) {
                path += ".json";
            }
            if (writeToPath(path)) {
                currentFilePath = path;
            }
        };

        const auto save = [writeToPath, saveAs] {
            if (currentFilePath.isEmpty()) {
                saveAs();
                return;
            }
            writeToPath(currentFilePath);
        };

        // TODO: add popup asking for confirmation
        const auto newScene = [glW, entityPropsWidget] {
            Scene &scene = glW->getScene();
            entityPropsWidget->setEntity(nullptr);
            scene.clearSelection();
            glW->getCameraController().clear();
            if (!scene.tryReset()) {
                QMessageBox::warning(
                    glW,
                    "New Scene",
                    "Some entities could not be removed and were left in place."
                );
            }
            setupDefaultCamerasAndCursor(glW);

            glW->getCommandStack().clear();
            currentFilePath.clear();

            emit glW->sceneChanged();
            glW->update();
        };

        const auto open = [glW, entityPropsWidget] {
            const QString path = QFileDialog::getOpenFileName(
                glW,
                "Open Scene",
                {},
                "Scene Files (*.json);;All Files (*)"
            );
            if (path.isEmpty()) {
                return;
            }
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(glW, "Open Failed", "Could not open file:\n" + path);
                return;
            }
            const auto doc = QJsonDocument::fromJson(file.readAll());

            if (QFile schemaFile(QCoreApplication::applicationDirPath() + "/format/schema.json");
                schemaFile.open(QIODevice::ReadOnly)) {
                const auto schema = QJsonDocument::fromJson(schemaFile.readAll());
                if (const auto errors = serialization::validateJson(schema, doc)) {
                    QString message = "Scene does not match the schema:\n";
                    for (const auto &[context, description] : *errors) {
                        message += "- " + QString::fromStdString(description) + "\n";
                    }
                    QMessageBox::warning(glW, "Open Failed", message);
                    return;
                }
            }

            Scene &scene = glW->getScene();
            entityPropsWidget->setEntity(nullptr);
            scene.clearSelection();
            PersistentEntities persistent;
            persistent.detachFrom(scene);
            if (!scene.tryReset()) {
                QMessageBox::warning(
                    glW,
                    "Open Failed",
                    "Some entities could not be removed; aborting to avoid a corrupted scene."
                );
                persistent.reattachTo(scene);
                return;
            }
            serialization::fromJson(scene, doc);
            persistent.reattachTo(scene);

            glW->getCommandStack().clear();
            currentFilePath = path;

            emit glW->sceneChanged();
            glW->update();
        };

        QObject::connect(menuBar, &CadMenuBar::newRequested, glW, newScene);
        QObject::connect(menuBar, &CadMenuBar::saveRequested, glW, save);
        QObject::connect(menuBar, &CadMenuBar::saveAsRequested, glW, saveAs);
        QObject::connect(menuBar, &CadMenuBar::openRequested, glW, open);
    }

    /// @brief Wires the Edit-menu Undo/Redo actions to the command stack and
    /// keeps their enabled state in sync when the menu opens
    void wireEditMenu(CadMenuBar *menuBar, OpenGlWidget *glWidget) {
        const auto undo =
            [glWidget] {
            glWidget->getCommandStack().undo();
        };
        const auto redo =
            [glWidget] {
            glWidget->getCommandStack().redo();
        };
        const auto refreshEnabled = [glWidget, menuBar] {
            menuBar->setUndoEnabled(glWidget->getCommandStack().canUndo());
            menuBar->setRedoEnabled(glWidget->getCommandStack().canRedo());
        };

        QObject::connect(menuBar, &CadMenuBar::undoRequested, glWidget, undo);
        QObject::connect(menuBar, &CadMenuBar::redoRequested, glWidget, redo);
        QObject::connect(glWidget, &OpenGlWidget::sceneChanged, menuBar, refreshEnabled);
        refreshEnabled();
    }

    /// @brief Keeps viewport, hierarchy and entity-properties selection/state in sync
    void wireSelectionSync(
        OpenGlWidget *glW,
        SceneHierarchyWidget *hierarchyWidget,
        EntityPropertiesWidget *entityPropsWidget
    ) {
        const auto vEntFromGl = [glW, entityPropsWidget] {
            const auto &sel = glW->getScene().getSelectedEntities();
            entityPropsWidget->setEntity(
                sel.size() == 1
                    ? *sel.begin()
                    : nullptr
            );
        };
        const auto vEntFromH = [entityPropsWidget](const QList<Entity*> &selected) {
            entityPropsWidget->setEntity(
                selected.size() == 1
                    ? selected.first()
                    : nullptr
            );
        };
        const auto glSelFromH = [glW](const QList<Entity*> &selected) {
            glW->getScene().clearSelection();
            for (auto *e : selected) {
                glW->getScene().setSelected(e, true);
            }
            glW->getScene().syncPointSelectionToRegistry();
            glW->update();
        };
        const auto onPropsChanged = [glW, hierarchyWidget] {
            hierarchyWidget->refresh();
            glW->update();
        };
        const auto onPointSelChanged = [glW, hierarchyWidget, entityPropsWidget](const QList<Entity*> &selected) {
            glW->getScene().clearSelection();
            for (auto *e : selected) {
                glW->getScene().setSelected(e, true);
            }
            glW->getScene().syncPointSelectionToRegistry();
            hierarchyWidget->syncSelectionFromScene();
            entityPropsWidget->syncBezierSelection();
            glW->update();
        };

        using namespace aliases;
        QObject::connect(glW, &GlW::sceneChanged, hierarchyWidget, &SceneHW::refresh);
        QObject::connect(glW, &GlW::viewportSelectionChanged, hierarchyWidget, &SceneHW::syncSelectionFromScene);
        QObject::connect(glW, &GlW::viewportSelectionChanged, entityPropsWidget, vEntFromGl);
        QObject::connect(hierarchyWidget, &SceneHW::selectionChanged, entityPropsWidget, vEntFromH);
        QObject::connect(hierarchyWidget, &SceneHW::selectionChanged, glW, glSelFromH);
        QObject::connect(entityPropsWidget, &EntPropsW::propertyChanged, glW, onPropsChanged);
        QObject::connect(entityPropsWidget, &EntPropsW::pointSelectionChanged, glW, onPointSelChanged);
        QObject::connect(glW, &GlW::viewportSelectionChanged, entityPropsWidget, &EntPropsW::syncBezierSelection);
        QObject::connect(glW, &GlW::sceneChanged, entityPropsWidget, &EntPropsW::refreshComponents);
        QObject::connect(glW, &GlW::geometryChanged, entityPropsWidget, &EntPropsW::refreshComponentGeometry);
    }

    /// @brief Wires the status line to viewport transform/selection/target/camera state
    void wireStatusBar(OpenGlWidget *glW, StatusBarWidget *statusBar) {
        const auto stBarWSetCamera = [statusBar](const std::string &name) {
            statusBar->setCameraName(QString::fromStdString(name));
        };
        const auto stBarWSetSelCount = [glW, statusBar] {
            statusBar->setSelectionCount(
                static_cast<int>(glW->getScene().getSelectedEntities().size())
            );
        };
        const auto stBarWSetNewPtsTargetName = [glW, statusBar] {
            if (const Entity *e = glW->getScene().getNewPointsTargetEntity()) {
                statusBar->setActiveNewPointsTargetName(QString::fromStdString(e->getName()));
            }
            else {
                statusBar->setActiveNewPointsTargetName({});
            }
        };

        using namespace aliases;
        QObject::connect(glW, &GlW::transformModeChanged, statusBar, &StBarW::setTransformMode);
        QObject::connect(glW, &GlW::clickToAddModeChanged, statusBar, &StBarW::setClickToAddMode);
        QObject::connect(&glW->getCameraController(), &CamContr::cameraChanged, statusBar, stBarWSetCamera);
        QObject::connect(glW, &GlW::viewportSelectionChanged, statusBar, stBarWSetSelCount);
        QObject::connect(glW, &GlW::sceneChanged, statusBar, stBarWSetNewPtsTargetName);
    }

    /// @brief Wires the viewport-panel controls (grid, pivot, coord space) and camera repaint
    void wireViewportControls(OpenGlWidget *glW, const ViewportPanelWidget *viewportPanel) {
        using namespace aliases;
        QObject::connect(viewportPanel->gridSettingsWidget(), &GridSW::gridPlanesChanged, glW, &GlW::setGridPlanes);
        glW->setGridPlanes(viewportPanel->gridSettingsWidget()->getGridPlanes());

        QObject::connect(viewportPanel->gridSettingsWidget(), &GridSW::axesMaskChanged, glW, &GlW::setInfiniteAxesMask);
        glW->setInfiniteAxesMask(viewportPanel->gridSettingsWidget()->getAxesMask());

        QObject::connect(viewportPanel->gridSettingsWidget(), &GridSW::lodFadeChanged, glW, &GlW::setGridLodFade);
        glW->setGridLodFade(viewportPanel->gridSettingsWidget()->getLodFade());

        auto *pivotCombo = viewportPanel->pivotCombo();
        const auto glSetPivotMode = [glW, pivotCombo](const int index) {
            glW->setPivotMode(static_cast<PivotMode>(pivotCombo->itemData(index).toInt()));
        };
        QObject::connect(pivotCombo, &QComboBox::currentIndexChanged, glW, glSetPivotMode);

        auto *coordSpaceCombo = viewportPanel->coordSpaceCombo();
        const auto glSetCoordSpace =
            [glW, coordSpaceCombo](const int index) {
            glW->setCoordSpace(static_cast<CoordSpace>(coordSpaceCombo->itemData(index).toInt()));
        };
        QObject::connect(coordSpaceCombo, &QComboBox::currentIndexChanged, glW, glSetCoordSpace);

        const auto glUpdate = [glW](const std::string &) {
            glW->update();
        };
        QObject::connect(&glW->getCameraController(), &CameraController::cameraChanged, glW, glUpdate);
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

    /// @brief Dims and input-blocks the whole window except the viewport,
    /// making a creator dialog behave modally while the camera stays navigable
    /// for the live preview
    /// @note Also closes the (parentless) dialog when the main
    /// window closes
    class ViewportModalOverlay final : public QWidget {
    public:
        ViewportModalOverlay(QWidget *window, QWidget *viewport, QDialog *dialog)
        : QWidget(window),
          m_viewport(viewport),
          m_dialog(dialog) {
            window->installEventFilter(this);
            setGeometry(window->rect());
            show();
            raise();
        }

        bool eventFilter(QObject *watched, QEvent *event) override {
            if (watched == parentWidget()) {
                if (event->type() == QEvent::Resize) {
                    setGeometry(parentWidget()->rect());
                }
                else if (event->type() == QEvent::Close) {
                    m_dialog->close();
                }
            }
            return QWidget::eventFilter(watched, event);
        }

    protected:
        void paintEvent(QPaintEvent *) override {
            QPainter p(this);
            p.fillRect(
                rect(),
                theme::active().dark
                    ? QColor(255, 255, 255, 36)
                    : QColor(0, 0, 0, 70)
            );
        }

        void resizeEvent(QResizeEvent *event) override {
            QWidget::resizeEvent(event);
            // cut the viewport out of the overlay
            const QRect vp(m_viewport->mapTo(parentWidget(), QPoint(0, 0)), m_viewport->size());
            setMask(QRegion(rect()) - QRegion(vp));
        }

    private:
        QWidget *m_viewport;
        QDialog *m_dialog;
    };

    /// @brief Wires entity-creation requests (torus/cursor/point + Bezier C0/C2) from both the
    /// hierarchy and the viewport into undoable command pushes
    void wireEntityCreation(OpenGlWidget *glW, const SceneHierarchyWidget *hierarchyWidget, const CadMenuBar *menuBar) {
        using namespace aliases;

        // ReSharper disable once CppDFAUnreachableFunctionCall
        auto spawnPos = [glW]() -> cadm::Vec3 {
            if (auto *activeCursor = glW->getScene().getActiveCursor()) {
                if (const auto t = activeCursor->getComponent<TransformComponent>()) {
                    return t.value()->getTranslation();
                }
            }
            return {};
        };

        auto spawnTorus = [glW, spawnPos] {
            glW->getCommandStack().push(
                std::make_unique<CreateEntityCommand>(
                    glW->getScene(),
                    [pos = spawnPos()](Scene &s) {
                        return GeometryFactory(s).createTorus(2.0f, 0.5f, 48, 24, pos);
                    }
                )
            );
        };

        auto spawnCursor = [glW, spawnPos] {
            glW->getCommandStack().push(
                std::make_unique<CreateEntityCommand>(
                    glW->getScene(),
                    [pos = spawnPos()](Scene &s) {
                        return GeometryFactory(s).createCursor(pos);
                    }
                )
            );
        };

        auto spawnPoint = [glW, spawnPos] {
            Scene &sc = glW->getScene();
            PointHandle createdHandle = InvalidPointHandle;
            glW->getCommandStack().push(
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
                glW->getCommandStack().push(
                    std::make_unique<AddControlPointCommand>(sc, target->getId(), createdHandle)
                );
            }
        };

        // ReSharper disable once CppDFAUnreachableFunctionCall
        auto collectSelectedPointHandles = [glW] {
            std::vector<PointHandle> handles;
            for (const auto &e : glW->getScene().getEntities()) {
                if (!e->isSelected()) {
                    continue;
                }
                if (const auto pc = e->getComponent<PointComponent>()) {
                    handles.push_back(pc.value()->m_handle);
                }
            }
            return handles;
        };

        auto spawnBezierC0 = [glW, collectSelectedPointHandles] {
            glW->getCommandStack().push(
                std::make_unique<CreateEntityCommand>(
                    glW->getScene(),
                    [handles = collectSelectedPointHandles()](Scene &s) {
                        return GeometryFactory(s).createBezierC0(handles, "BezierC0");
                    }
                )
            );
        };

        auto spawnBezierC2 = [glW, collectSelectedPointHandles] {
            glW->getCommandStack().push(
                std::make_unique<CreateEntityCommand>(
                    glW->getScene(),
                    [handles = collectSelectedPointHandles()](Scene &s) {
                        return GeometryFactory(s).createBezierC2(handles, "BezierC2");
                    }
                )
            );
        };

        auto spawnInterpC2 = [glW, collectSelectedPointHandles] {
            glW->getCommandStack().push(
                std::make_unique<CreateEntityCommand>(
                    glW->getScene(),
                    [handles = collectSelectedPointHandles()](Scene &s) {
                        return GeometryFactory(s).createInterpC2(handles, "InterpC2");
                    }
                )
            );
        };

        // ReSharper disable once CppDFAUnreachableFunctionCall
        auto spawnPatch = [glW, menuBar](const bool c2) {
            // one creator dialog at a time; re-request just raises the open one
            static QPointer<PatchCreatorDialog> open;
            if (open) {
                open->raise();
                open->activateWindow();
                return;
            }
            const auto dialog = new PatchCreatorDialog(c2, nullptr);
            open = dialog;
            dialog->setWindowFlag(Qt::WindowStaysOnTopHint);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            // ReSharper disable once CppDFAMemoryLeak
            const auto overlay = new ViewportModalOverlay(glW->window(), glW, dialog);
            menuBar->setFileActionsEnabled(false);
            const auto onAccept = [glW, dialog, overlay, menuBar](const int result) {
                overlay->deleteLater();
                menuBar->setFileActionsEnabled(true);
                glW->clearPatchPreview();
                if (result == QDialog::Accepted) {
                    auto params = dialog->params();
                    std::tie(params.origin, params.orientation) = glW->activeCursorPlacement();
                    glW->getCommandStack().push(
                        std::make_unique<CreatePatchCommand>(glW->getScene(), params)
                    );
                }
            };
            QObject::connect(dialog, &PCDialog::paramsChanged, glW, &GlW::setPatchPreview);
            QObject::connect(dialog, &PCDialog::showNetChanged, glW, &GlW::setPatchPreviewShowNet);
            QObject::connect(dialog, &PCDialog::hideSceneChanged, glW, &GlW::setPatchPreviewHideScene);
            QObject::connect(dialog, &QDialog::finished, glW, onAccept);
            dialog->show();
        };

        QObject::connect(hierarchyWidget, &SceneHW::createTorusRequested, glW, spawnTorus);
        QObject::connect(hierarchyWidget, &SceneHW::createCursorRequested, glW, spawnCursor);
        QObject::connect(hierarchyWidget, &SceneHW::createPointRequested, glW, spawnPoint);
        QObject::connect(glW, &GlW::createTorusRequested, glW, spawnTorus);
        QObject::connect(glW, &GlW::createCursorRequested, glW, spawnCursor);
        QObject::connect(glW, &GlW::createPointRequested, glW, spawnPoint);

        // Bezier C0 signals
        QObject::connect(hierarchyWidget, &SceneHW::createBezierC0Requested, glW, spawnBezierC0);
        QObject::connect(glW, &GlW::createBezierC0Requested, glW, spawnBezierC0);

        const auto setEntAsNewPtsTarget =
            [glW](Entity *e) {
            glW->getScene().setNewPointsTargetEntity(e);
            emit glW->sceneChanged();
        };
        QObject::connect(hierarchyWidget, &SceneHW::setAsNewPointsTargetEntityRequested, glW, setEntAsNewPtsTarget);

        const auto addSelPts = [glW](const Entity *entity) {
            if (!entity->hasComponent<INewPointsTargetBase>()) {
                return;
            }
            Scene &sc = glW->getScene();
            const EntityId curveId = entity->getId();
            for (const auto &e : sc.getEntities()) {
                if (!e->isSelected()) {
                    continue;
                }
                if (const auto pc = e->getComponent<PointComponent>()) {
                    glW->getCommandStack().push(
                        std::make_unique<AddControlPointCommand>(sc, curveId, pc.value()->m_handle)
                    );
                }
            }
        };
        QObject::connect(hierarchyWidget, &SceneHW::addSelectedPointsToNewPointsTargetEntityRequested, glW, addSelPts);

        // Bézier C2 signals
        QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createBezierC2Requested, glW, spawnBezierC2);
        QObject::connect(glW, &OpenGlWidget::createBezierC2Requested, glW, spawnBezierC2);

        // interpolating C2 signals
        QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createInterpC2Requested, glW, spawnInterpC2);
        QObject::connect(glW, &OpenGlWidget::createInterpC2Requested, glW, spawnInterpC2);

        // Bézier patch signals
        const auto spawnPatchC0 = [spawnPatch] {
            spawnPatch(false);
        };
        const auto spawnPatchC2 = [spawnPatch] {
            spawnPatch(true);
        };
        QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createPatchC0Requested, glW, spawnPatchC0);
        QObject::connect(glW, &OpenGlWidget::createPatchC0Requested, glW, spawnPatchC0);
        QObject::connect(hierarchyWidget, &SceneHierarchyWidget::createPatchC2Requested, glW, spawnPatchC2);
        QObject::connect(glW, &OpenGlWidget::createPatchC2Requested, glW, spawnPatchC2);
    }
}

int main(int argc, char *argv[]) {
    using namespace aliases;

    Q_INIT_RESOURCE(resources);

    glSetDefaults();
    [[maybe_unused]] QApplication a(argc, argv);

    // load the bundled JetBrains Mono and make it the default application font;
    // the family name is read back from the loaded font rather than hard-coded
    QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Bold.ttf");
    if (const int fontId = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf");
        fontId != -1) {
        if (const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            !families.isEmpty()) {
            QFont appFont(families.first());
            appFont.setPointSize(11);
            QApplication::setFont(appFont);
        }
    }

    theme::apply(theme::gc_light);

    QWidget window;
    window.setWindowFlag(Qt::FramelessWindowHint);
    window.setMinimumSize(QSize(500, 500));

    // window paints its (now theme-seeded) QPalette::Window background
    window.setAutoFillBackground(true);

    // ReSharper disable once CppDFAMemoryLeak
    const auto outerLayout = new QVBoxLayout(&window);
    outerLayout->setSpacing(gc_resizeMargin);
    outerLayout->setContentsMargins(gc_resizeMargin, gc_resizeMargin, gc_resizeMargin, gc_resizeMargin);

    const auto menuBar = new CadMenuBar;
    const auto titleBar = new CadTitleBar(menuBar);
    outerLayout->addWidget(titleBar);

    // ReSharper disable once CppDFAMemoryLeak
    const auto content = new QWidget;
    content->setCursor(Qt::ArrowCursor);
    // ReSharper disable once CppDFAMemoryLeak
    const auto rootLayout = new QHBoxLayout(content);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(content, 1);

    // ReSharper disable once CppDFAMemoryLeak
    const auto leftContainer = new QWidget;
    // ReSharper disable once CppDFAMemoryLeak
    const auto leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setSpacing(gc_separatorStripWidth);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    const auto glW = new OpenGlWidget;
    glW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    glW->setMinimumWidth(40);
    leftLayout->addWidget(glW);

    menuBar->applyShortcuts(glW->getInputMap());

    const auto statusBar = new StatusBarWidget;
    statusBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    const auto scenePanel = new ScenePanelWidget;
    const auto viewportPanel = new ViewportPanelWidget;
    const auto propertiesPanel = new PropertiesPanelWidget;

    scenePanel->setFocusPolicy(Qt::ClickFocus);
    viewportPanel->setFocusPolicy(Qt::ClickFocus);
    propertiesPanel->setFocusPolicy(Qt::ClickFocus);

    // ReSharper disable once CppDFAMemoryLeak
    const auto panelStack = new SubdividedPanelBar;
    panelStack->registerPanel(0, scenePanel);
    panelStack->registerPanel(1, viewportPanel);
    panelStack->registerPanel(2, propertiesPanel);
    panelStack->setMinimumWidth(200);

    prepareToolPanels({scenePanel, viewportPanel, propertiesPanel});

    // ReSharper disable once CppDFAMemoryLeak
    const auto splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(gc_separatorStripWidth);
    splitter->addWidget(leftContainer); // index 0: stretches
    splitter->addWidget(panelStack); // index 1: resizable, collapsible
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({10000, 450});
    rootLayout->addWidget(splitter, 1);

    const auto toolsPanelBar = new ToolPanelBar;
    rootLayout->addWidget(toolsPanelBar, 0);

    outerLayout->addWidget(statusBar);

    const auto *sceneAction = menuBar->addToolPanelAction("Scene");
    const auto *viewportAction = menuBar->addToolPanelAction("Viewport");
    const auto *propertiesAction = menuBar->addToolPanelAction("Properties");

    auto *sceneBtn = toolsPanelBar->addPanel("Scene");
    auto *viewportBtn = toolsPanelBar->addPanel("Viewport");
    auto *propertiesBtn = toolsPanelBar->addPanel("Properties", false);

    wirePanelToggles(
        toolsPanelBar,
        panelStack,
        {
            {sceneAction, sceneBtn, 0},
            {viewportAction, viewportBtn, 1},
            {propertiesAction, propertiesBtn, 2},
        }
    );

    toolsPanelBar->openPanel(2);
    toolsPanelBar->openPanel(0);

    // intelliJ inspired active-tab accent: the open tab paints accent color
    // only while its tool panel holds focus, falling back to the hover color
    // otherwise; several tabs can be open at once but at most one is focused
    const auto syncActiveTabHighlight = [toolsPanelBar, panelStack](QWidget *, const QWidget *now) {
        const QWidget *popup = QApplication::activePopupWidget();
        const QWidget *effective = popup != nullptr && popup->parentWidget() != nullptr
                                       ? popup->parentWidget()
                                       : now;
        const auto focused = panelStack->focusedPanelIndex(effective).value_or(-1);
        for (int i = 0; i < toolsPanelBar->count(); ++i) {
            toolsPanelBar->setPanelFocused(i, i == focused);
        }
    };
    QObject::connect(qApp, &QApplication::focusChanged, toolsPanelBar, syncActiveTabHighlight);

    auto *hierarchyWidget = scenePanel->hierarchyWidget();
    auto *entityPropertiesWidget = propertiesPanel->entityPropertiesWidget();

    setupDefaultCamerasAndCursor(glW);
    statusBar->setCameraName(QString::fromStdString(glW->getCameraController().getActiveName()));

    hierarchyWidget->setScene(&glW->getScene());
    hierarchyWidget->setCommandStack(&glW->getCommandStack());
    hierarchyWidget->setCameraController(&glW->getCameraController());
    entityPropertiesWidget->setScene(&glW->getScene());
    entityPropertiesWidget->setCommandStack(&glW->getCommandStack());

    const auto updateAppTheme = [glW](const bool dark) {
        theme::apply(
            dark
                ? theme::gc_dark
                : theme::gc_light
        );
        glW->update();
    };
    QObject::connect(menuBar, &CadMenuBar::darkThemeChanged, glW, updateAppTheme);
    QObject::connect(menuBar, &CadMenuBar::stereoEnabledChanged, glW, &GlW::setStereoEnabled);
    QObject::connect(menuBar, &CadMenuBar::stereoAutoChanged, glW, &GlW::setStereoAuto);
    QObject::connect(menuBar, &CadMenuBar::stereoLuminanceChanged, glW, &GlW::setStereoLuminance);
    QObject::connect(menuBar, &CadMenuBar::stereoAutoEyeSepChanged, glW, &GlW::setStereoAutoEyeSep);
    QObject::connect(menuBar, &CadMenuBar::stereoSepRatioChanged, glW, &GlW::setStereoSeparationRatio);
    QObject::connect(menuBar, &CadMenuBar::stereoEyeSeparationChanged, glW, &GlW::setStereoEyeSeparation);
    QObject::connect(menuBar, &CadMenuBar::stereoConvergenceChanged, glW, &GlW::setStereoConvergence);
    QObject::connect(glW, &GlW::stereoEyeSepChanged, menuBar, &CadMenuBar::setStereoEyeSep);
    QObject::connect(glW, &GlW::stereoConvergenceChanged, menuBar, &CadMenuBar::setStereoConvergence);

    wireFileMenu(menuBar, glW, entityPropertiesWidget);
    wireEditMenu(menuBar, glW);
    wireSelectionSync(glW, hierarchyWidget, entityPropertiesWidget);
    wireStatusBar(glW, statusBar);
    wireViewportControls(glW, viewportPanel);
    wireHierarchyActions(glW, hierarchyWidget);
    wireEntityCreation(glW, hierarchyWidget, menuBar);

    QApplication::instance()->installEventFilter(glW);
    enableFramelessResize(&window, gc_resizeMargin);
    window.show();
    return QApplication::exec();
}
