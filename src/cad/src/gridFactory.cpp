//
// Created for Grid and Axes generation.
//

#include "gridFactory.hpp"
// Assuming you have some sort of mesh and material components:
#include "components/meshComponent.hpp"
#include "components/transformComponent.hpp"

entity* GridFactory::createInfiniteGrid(const std::string &name) const
{
    const auto entity = m_scene.createEntity(name);
    const auto mesh = entity->addComponent<MeshComponent>();

    // Set up a large quad on the XZ plane. 
    // In a pure infinite shader, this could also be a full-screen quad where you unproject the depth.
    mesh->setQuadGeometry(10000.0f); // Make it massive

    // This material would use a custom grid shader (example provided below)
    mesh->setMaterial("InfiniteGridMaterial");

    return entity;
}

entity* GridFactory::createWorldAxes(const std::string &name) const
{
    const auto entity = m_scene.createEntity(name);
    const auto mesh = entity->addComponent<MeshComponent>();

    // Pseudo-code for setting up GL_LINES geometry with colors
    // Format: Start Point, End Point, Color (R, G, B)
    mesh->setLineGeometry(
        {
            {cadm::vec3(0, 0, 0), cadm::vec3(100, 0, 0), cadm::vec3(1, 0, 0)},
            // X-Axis (Red)
            {cadm::vec3(0, 0, 0), cadm::vec3(0, 100, 0), cadm::vec3(0, 1, 0)},
            // Y-Axis (Green)
            {cadm::vec3(0, 0, 0), cadm::vec3(0, 0, 100), cadm::vec3(0, 0, 1)} // Z-Axis (Blue)
        });

    return entity;
}