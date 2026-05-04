//
// Created on 4/1/26.
//

#ifndef CAD_ENTITYSNAPSHOT_HPP
#define CAD_ENTITYSNAPSHOT_HPP
#include "Entity.hpp"
#include "PointComponent.hpp"
#include "TransformComponent.hpp"
#include "cad_math/helpers.hpp"
#include "cad_math/mat3.hpp"
#include "cad_math/vec3.hpp"

struct EntitySnapshot
{
    EntityID id{};
    cadm::vec3 origPos;
    cadm::mat3 origRotMat;
    cadm::vec3 origScale;
    bool isTransformEntity{false}; // false for points

    void restoreEntity(PointRegistry &pointRegistry, Entity *entity) const;
    bool fillFromEntity(const PointRegistry &pointRegistry, Entity *entity);

private:
    void fillFromPointComponent(const PointRegistry &pointRegistry, PointComponent *pointComponent);
    void fillFromTransformComponent(const TransformComponent *transformComponent);
};

inline void EntitySnapshot::restoreEntity(PointRegistry &pointRegistry, Entity *entity) const
{
    if (!isTransformEntity)
    {
        if (const auto pc = entity->getComponent<PointComponent>())
            pointRegistry.setPosition(pc.value()->m_handle, origPos);
    }
    else
    {
        if (const auto tc = entity->getComponent<TransformComponent>())
        {
            tc.value()->setTranslation(origPos);
            tc.value()->setRotation(cadm::eulerZYXFromRotMat(origRotMat));
            tc.value()->setScale(origScale);
        }
    }
}

inline bool EntitySnapshot::fillFromEntity(const PointRegistry &pointRegistry, Entity *entity)
{
    id = entity->getId();
    if (const auto pc = entity->getComponent<PointComponent>())
    {
        fillFromPointComponent(pointRegistry, pc.value());
        return true;
    }
    if (const auto tc = entity->getComponent<TransformComponent>())
    {
        fillFromTransformComponent(tc.value());
        return true;
    }
    return false;
}

inline void EntitySnapshot::fillFromPointComponent(const PointRegistry &pointRegistry, PointComponent *pointComponent)
{
    origPos = pointRegistry.getPosition(pointComponent->m_handle);
    origRotMat = cadm::mat3::identity();
    origScale = {1, 1, 1};
    isTransformEntity = false;
}

inline void EntitySnapshot::fillFromTransformComponent(const TransformComponent *transformComponent)
{
    origPos = transformComponent->getTranslation();
    origScale = transformComponent->getScale();
    origRotMat = transformComponent->getModelMatrix().upperLeft3x3().normalizedColumns();
    isTransformEntity = true;
}

#endif //CAD_ENTITYSNAPSHOT_HPP
