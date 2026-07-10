#ifndef SCENEHIERARCHYWIDGET_H
#define SCENEHIERARCHYWIDGET_H

#include <QListWidgetItem>
#include <QPushButton>

#include "../../../components/Entity.hpp"
#include "../../../Scene.hpp"
#include "../../../camera/CameraController.hpp"
#include "components/ComponentChecker.hxx"

class CommandStack;
class SceneFiltersPopup;
class SceneHierarchyWidget;

namespace aliases {
    // ReSharper disable once CppInconsistentNaming
    using SceneHW = SceneHierarchyWidget;
}

/// @brief Widget displaying the list of the entities from the scene
class SceneHierarchyWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SceneHierarchyWidget(QWidget *parent = nullptr);

    /// @brief Sets the scene that is the source of the entities' data
    void setScene(Scene *scene);

    /// @brief Sets the command stack so renames become undoable
    void setCommandStack(CommandStack *stack) {
        m_commandStack = stack;
    }

    /// @brief Sets the CameraController for the scene
    /// @param cameraController the CameraController for the scene
    void setCameraController(CameraController *cameraController);

signals:
    void selectionChanged(QList<Entity*> entities);

    void deleteEntityRequested(Entity *e);

    void setAsCursorRequested(Entity *e);

    void setAsCameraRequested(EntityId id);

    void focusCameraRequested(Entity *e);

    void createTorusRequested();

    void createCursorRequested();

    void createPointRequested();

    void createBezierC0Requested();

    void createBezierC2Requested();

    void createInterpC2Requested();

    void createPatchC0Requested();

    void createPatchC2Requested();

    void createGregoryRequested();

    void setAsNewPointsTargetEntityRequested(Entity *e);

    void addSelectedPointsToNewPointsTargetEntityRequested(Entity *e);

    /// @brief Collapse the two currently selected (point) entities into one
    void collapseSelectedPointsRequested();

public
slots:
    /// @brief Update the m_listWidget to be in sync with the m_scene entities
    void refresh();

    /// @brief Sync the selection of the m_listWidget list items to be in sync with the m_scene
    /// (m_scene is the source of the selection truth here)
    void syncSelectionFromScene();

private
slots:
    void onItemSelectionChanged();

    void onItemChanged(const QListWidgetItem *item) const;

    /// @brief Show the context menu with generic entity actions (add new entity etc.)
    /// @param pos position for the context menu
    void onContextMenuRequested(const QPoint &pos);

private:
    void onFiltersPopupRequested() const;

    void onFiltersChanged();

    bool matchesAnyFilter(Entity *e) const;

    /// @brief Helper to add entity to the m_listWidget
    void addEntityToList(const std::unique_ptr<Entity> &e) const;

    /// @brief Populate the m_listWidget with m_scene entities
    /// @note compared to syncSelectionFromScene, this clears the m_listWidget list and populated from scratch
    void populateList();

    /// @brief Whether the data is refreshing
    /// @note this ensures infinite call loops are avoided
    bool m_refreshing{false};

    /// @brief The Scene being the source of truth about entities
    Scene *m_scene = nullptr;

    /// @brief Command stack for undoable renames
    /// @note optional
    CommandStack *m_commandStack = nullptr;

    /// @brief The CameraController for the scene
    CameraController *m_cameraController = nullptr;

    /// @brief List widget where each list item represents a single m_scene entity
    QListWidget *m_listWidget;

    QPushButton *m_filtersButton;
    SceneFiltersPopup *m_filtersPopup;

    ComponentFilters m_filters{};
};

#endif // SCENEHIERARCHYWIDGET_H
