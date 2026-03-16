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
#include <cstdint>
#include <optional>
#include <ranges>
#include <type_traits>
#include <QMetaType>

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
    void setName(const std::string &name) { m_name = name; }

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

Q_DECLARE_METATYPE(entity *)

template <typename T>
T* entity::addComponent()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    auto component = std::make_unique<T>();
    T *ptr = component.get();
    m_components[std::type_index(typeid(T))] = std::move(component);
    return ptr;
}

template <typename T>
std::optional<T*> entity::getComponent()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    for (const auto &val : m_components | std::views::values)
    {
        if (T *component_ptr = dynamic_cast<T*>(val.get()))
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
    return std::ranges::any_of(
        m_components.begin(),
        m_components,
        [&](const auto &component)
        {
            return dynamic_cast<T*>(component.get());
        });
}

template <typename T>
void entity::removeComponent()
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    m_components.erase(std::type_index(typeid(T)));
}

#endif //CAD_ENTITY_H