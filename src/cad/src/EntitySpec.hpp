//
// Created on 6/19/26.
//

#ifndef CAD_ENTITYSPEC_HPP
#define CAD_ENTITYSPEC_HPP

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

#include <cad_math/vec3.hpp>

#include "PointRegistry.hpp"
#include "components/Entity.hpp"
#include "BSplineToBezierConverter.hpp" // ParametrizationMode

class Scene;

// one struct per-component kind.
// An entity is described by a *list* of these, mirroring the runtime component bag
// (an entity may hold any combination)

struct TransformData {
    cadm::vec3 translation{};
    cadm::vec3 rotation{};
    cadm::vec3 scale{1, 1, 1};
};

struct PointData {
    PointHandle handle{InvalidPointHandle};
    cadm::vec3 position{};
};

struct TorusData {
    cadm::cadf majorRadius{2};
    cadm::cadf minorRadius{0.5f};
    uint32_t majorSegments{48};
    uint32_t minorSegments{24};
};

struct AxesData {
    float length{5.0f};
};

struct CursorData {};

struct BezierC0Data {
    std::vector<PointHandle> controlPoints;
    bool showPolygon{false};
};

struct BezierC2Data {
    std::vector<PointHandle> controlPoints;
    bool showDeBoorPolygon{true};
    bool showBernsteinPolygon{false};
    ParametrizationMode parametrization{ParametrizationMode::chordLength};
};

using ComponentSpec =
std::variant<TransformData, PointData, TorusData, AxesData, CursorData, BezierC0Data, BezierC2Data>;

/// @brief 
/// Plain-data description of an entity: 
/// identity plus the list of components it holds. 
/// Sufficient to rebuild the entity exactly (same EntityID / PointHandle)
struct EntitySpec {
    EntityId id{};
    std::string name;
    bool visible{true};
    std::vector<ComponentSpec> components;

    [[nodiscard]] bool isPoint() const {
        return std::ranges::any_of(
            components,
            [](const ComponentSpec &c) {
                return std::holds_alternative<PointData>(c);
            }
        );
    }
};

/// @brief Capture an entity into a spec. Returns false if no serializable component was found
/// @param scene The scene containing the entity
/// @param entity The entity to capture
/// @param out The output spec to populate with entity data
/// @return true if the entity was successfully captured, false if no serializable component was found
bool captureEntity(Scene & scene, Entity * entity, EntitySpec & out);

/// @brief Rebuild an entity from a spec, preserving its id / handle
/// @param scene The scene in which to rebuild the entity
/// @param spec The entity specification containing the data to rebuild from
/// @return Pointer to the rebuilt entity, or nullptr if rebuild failed
Entity* rebuildEntity(Scene &scene, const EntitySpec &spec);

#endif //CAD_ENTITYSPEC_HPP
