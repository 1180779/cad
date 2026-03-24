//
// Created for Grid and Axes generation.
//

#pragma once

#include <string>
#include "scene.hpp" // Assuming this is where m_scene comes from
#include "math/cadMath.hpp" // Assuming cadm::vec3 lives in a similar header

class GridFactory
{
public:
    explicit GridFactory(Scene &scene)
        : m_scene(scene)
    {
    }

    // Creates the massive plane for the infinite grid shader
    entity* createInfiniteGrid(const std::string &name = "InfiniteGrid") const;

    // Creates the Red, Green, Blue lines at the origin
    entity* createWorldAxes(const std::string &name = "WorldAxes") const;

private:
    Scene &m_scene;
};