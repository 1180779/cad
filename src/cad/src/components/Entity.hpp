//
// Created by rdkgsk on 3/11/26.
//

#ifndef CAD_ENTITY_H
#define CAD_ENTITY_H

#include <algorithm>
#include <string>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <QMetaType>

using EntityID = uint32_t;

/// Opaque token that only Scene can construct, used to restrict Entity::setSelected
/// to Scene::setSelected / Scene::clearSelection callers.
class SelectionKey final {
    SelectionKey() = default;

    friend class Scene;
};

class Component {
public:
    virtual ~Component() = default;

    bool enabled{true};
};

class Entity {
public:
    explicit Entity(const EntityID id, std::string name = "Entity") : m_id(id), m_name(std::move(name)) {}

    ~Entity() = default;

    EntityID getId() const {
        return m_id;
    }

    const std::string& getName() const {
        return m_name;
    }

    void setName(const std::string &name) {
        m_name = name;
    }

    bool isVisible() const {
        return m_visible;
    }

    void setVisible(const bool visible) {
        m_visible = visible;
    }

    bool isSelected() const {
        return m_selected;
    }

    void setSelected(const bool selected, SelectionKey) {
        m_selected = selected;
    }

    template <typename T, typename... Args>
    T* addComponent(Args &&... args);

    /// returns the component of the entity or [std::nullopt] if the entity does not have a component of the desired type
    template <typename T>
    std::optional<T*> getComponent();

    template <typename T>
    bool hasComponent() const;

    template <typename T>
    void removeComponent();

private:
    EntityID m_id;
    std::string m_name;
    bool m_visible{true};
    bool m_selected{false};
    std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
};

Q_DECLARE_METATYPE(Entity *)

template <typename T, typename... Args>
T* Entity::addComponent(Args &&... args) {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    T *ptr = component.get();
    m_components[std::type_index(typeid(T))] = std::move(component);
    return ptr;
}

template <typename T>
std::optional<T*> Entity::getComponent() {
    if constexpr (std::is_base_of_v<Component, T>) {
        if (const auto it = m_components.find(std::type_index(typeid(T)));
            it != m_components.end()) {
            return static_cast<T*>(it->second.get());
        }
    }

    for (const auto &val : m_components | std::views::values) {
        if (T *ptr = dynamic_cast<T*>(val.get())) {
            return ptr;
        }
    }

    return std::nullopt;
}

template <typename T>
bool Entity::hasComponent() const {
    if constexpr (std::is_base_of_v<Component, T>) {
        if (m_components.contains(std::type_index(typeid(T)))) {
            return true;
        }
    }

    return std::ranges::any_of(
        m_components | std::views::values,
        [](const auto &val) {
            return dynamic_cast<T*>(val.get()) != nullptr;
        }
    );
}

template <typename T>
void Entity::removeComponent() {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    m_components.erase(std::type_index(typeid(T)));
}

#endif //CAD_ENTITY_H
