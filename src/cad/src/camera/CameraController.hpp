//
// Created on 3/26/26.
//

#ifndef CAD_CAMERACONTROLLER_HPP
#define CAD_CAMERACONTROLLER_HPP

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include <QObject>

#include "ICameraStrategy.hpp"
#include "../components/PointComponent.hpp"
#include "../components/TransformComponent.hpp"
#include "../PointRegistry.hpp"

/// @brief Manages the available ICameraStrategy and syncs them with scene entity changes
class CameraController final : public QObject {
    Q_OBJECT

public:
    struct CameraStrategyEntry {
        std::string name;
        std::unique_ptr<ICameraStrategy> strategy;
    };

    explicit CameraController(QObject *parent = nullptr);

    /// @brief Add the strategy to the available strategies
    /// @param name name to be associated with the strategy
    /// @param strategy the strategy to be added
    void addCamera(std::string name, std::unique_ptr<ICameraStrategy> strategy);

    /// @brief Remove a strategy borrowing an entity
    /// @param id id of the entity
    /// @note does not remove the strategy if it is the last one available
    void removeCamera(EntityID id);

    /// @brief Get the active strategy
    /// @return the active strategy
    [[nodiscard]] ICameraStrategy* getActiveStrategy() const;

    /// @brief Get the name of the active strategy
    /// @return std::string& of the active strategy name
    [[nodiscard]] const std::string& getActiveName() const;

    /// @brief Switch to the next available strategy
    void switchToNext();

    /// @brief Switch to the strategy with the provided name
    /// @param name of the strategy to use
    void switchTo(const std::string &name);

    /// @brief Switch to the strategy that borrows the provided entity
    /// @param id id of the entity
    void switchTo(EntityID id);

    /// @brief Whether an entity is borrowed by the current strategy
    /// @param id id of the entity
    /// @return true when the entity is referenced by current strategy, false otherwise
    [[nodiscard]] bool isActiveCamera(const EntityID id) const;

    /// @brief Whether an entity is borrowed by any of the available strategies
    /// @param id id of the entity
    /// @return true when the entity is referenced by any strategy, false otherwise
    [[nodiscard]] bool isEntityManagedAsCamera(EntityID id) const;

    /// @brief Sets the current camera to look in the direction of the specified entity
    /// @param entity the target entity
    /// @param registry PointRegistry, which manages the connected PointHandle if the Entity has a PointComponent
    /// @note prioritizes the first PointComponent of the entity as the direction source
    void lookAtEntity(Entity *entity, const PointRegistry &registry) const;

    /// @brief Get available camera strategies
    /// @return available camera strategies
    [[nodiscard]] const std::vector<CameraStrategyEntry>& getStrategies() const {
        return m_cameras;
    }

signals :
    void cameraChanged(const std::string &name);

private:
    /// @brief Available camera strategies
    std::vector<CameraStrategyEntry> m_cameras;

    /// @brief Index of the currently active camera strategy (in m_cameras)
    std::size_t m_activeIndex{0};
};

#endif //CAD_CAMERACONTROLLER_HPP
