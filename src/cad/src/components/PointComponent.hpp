//
// Created on 3/31/26.
//

#ifndef CAD_POINTCOMPONENT_HPP
#define CAD_POINTCOMPONENT_HPP

#include "Entity.hpp"
#include "../PointRegistry.hpp"

class PointComponent final : public Component
{
public:
    explicit PointComponent(const PointHandle handle)
        : m_handle(handle)
    {
    }

    PointHandle m_handle;
};

#endif //CAD_POINTCOMPONENT_HPP
