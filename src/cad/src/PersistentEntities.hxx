//
// Created by Radosław Głasek on 07.07.2026
//

#ifndef CAD_PERSISTENTENTITIES_HXX
#define CAD_PERSISTENTENTITIES_HXX

#include <memory>
#include <vector>

#include "components/Entity.hpp"

class Scene;

/// @brief True for entities that live outside the serialized scene format
/// (cameras, cursor) and must survive Open/New rather than being wiped along
/// with old geometry
bool isPersistentEntity(const Entity *e);

/// @brief Detaches the persistent entities from the scene, so a fresh document
/// can be loaded with its own ids reproduced exactly then hands them back with
/// freshly allocated, non-colliding ids
/// @see @c isPersistentEntity
class PersistentEntities final {
public:
    void detachFrom(Scene &scene);

    void reattachTo(Scene &scene);

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
};

#endif //CAD_PERSISTENTENTITIES_HXX
