//
// Created on 3/15/26.
//

#include "scene.h"

#include <ranges>
#include <algorithm>

entity* Scene::createEntity(const std::string& name)
{
    EntityID entityId = m_nextEntityId++;
    m_entities.emplace_back(std::make_unique<entity>(entityId, name));
    return &*m_entities.back();
}

std::optional<entity*> Scene::getEntity(EntityID id)
{
    const auto result = std::ranges::find_if(
        m_entities,
        [id](const std::unique_ptr<entity>& e)
        {
            return e->getId() == id;
        });
    if (result != m_entities.end()) return result->get();
    return std::nullopt;
}

std::optional<entity*> Scene::getEntityByName(const std::string& name)
{
    const auto byName = std::ranges::find_if(
        m_entities,
        [name](const std::unique_ptr<entity>& e)
        {
            return e->getName() == name;
        });
    if (byName != m_entities.end()) return byName->get();
    return std::nullopt;
}

void Scene::removeEntity(EntityID id)
{
    // pop and replace
    const auto toBeRemoved = std::ranges::find_if(
        m_entities,
        [id](const std::unique_ptr<entity>& e)
        {
            return e->getId() == id;
        });
    if (toBeRemoved == m_entities.end()) return;
    toBeRemoved->swap(m_entities.back());
    m_entities.pop_back();
}

auto Scene::getVisibleEntities()
{
    return m_entities | std::views::filter(
        [](const std::unique_ptr<entity>& e)
        {
            return e->isVisible();
        });
}

auto Scene::getSelectedEntities()
{
    return m_entities | std::views::filter(
        [](const std::unique_ptr<entity>& e)
        {
            return e->isSelected();
        });
}
