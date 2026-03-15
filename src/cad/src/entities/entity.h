//
// Created by rdkgsk on 3/11/26.
//

#ifndef CAD_ENTITY_H
#define CAD_ENTITY_H

#include <string>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <cstdint>
#include <optional>
#include <type_traits> // Required for std::is_base_of_v

using EntityID = uint32_t;

class Component
{
public:
    virtual ~Component() = default;
    bool enabled{true};
};

class entity
{
public:
    explicit entity(const EntityID id, std::string name = "Entity")
        : m_id(id), m_name(std::move(name))
    {
    }

    ~entity() = default;

    EntityID getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    bool isVisible() const { return m_visible; }
    void setVisible(const bool visible) { m_visible = visible; }

    bool isSelected() const { return m_selected; }
    void setSelected(const bool selected) { m_selected = selected; }

    template <typename T>
    T* addComponent();

    // returns the component of the entity or [std::nullopt] if the entity does not have a component of the desired type
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

template <typename T>
T* entity::addComponent()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    auto component = std::make_unique<T>();
    T* ptr = component.get();
    // Store by the exact type of the component being added
    m_components[std::type_index(typeid(T))] = std::move(component);
    return ptr;
}

template <typename T>
std::optional<T*> entity::getComponent()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    // Iterate through all components and try to dynamic_cast
    for (const auto& pair : m_components)
    {
        if (T* component_ptr = dynamic_cast<T*>(pair.second.get()))
        {
            return component_ptr;
        }
    }
    return std::nullopt;
}

template <typename T>
bool entity::hasComponent() const
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    // Iterate through all components and try to dynamic_cast
    for (const auto& pair : m_components)
    {
        if (dynamic_cast<T*>(pair.second.get()))
        {
            return true;
        }
    }
    return false;
}

template <typename T>
void entity::removeComponent()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    // Remove by the exact type
    m_components.erase(std::type_index(typeid(T)));
}

#endif //CAD_ENTITY_H
