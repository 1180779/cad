//
// Created on 3/17/26.
//

#ifndef CAD_CAMERA_HPP
#define CAD_CAMERA_HPP

#include "../entities/entity.h"
#include "cad_math/mat4.h"
#include "cad_math/vec3.h"

class CameraComponent final : public Component
{
public:
    static constexpr cadm::cadf s_minDistance = 0.01;
    static constexpr cadm::cadf s_minDistanceSq = s_minDistance * s_minDistance;

    cadm::vec3 forward() const;
    cadm::vec3 right() const;
    cadm::vec3 up() const;

    cadm::vec3 m_position{};
    cadm::vec3 m_target{};
    cadm::vec3 m_worldUp = cadm::vec3::unitY();

    cadm::cadf m_aspectRatio{1.0f};
    cadm::cadf m_nearPlane{0.1f};
    cadm::cadf m_farPlane{100.0f};

    cadm::cadf m_orthoHeight{2.0f};
};

inline cadm::vec3 CameraComponent::forward() const
{
    return (m_target - m_position).normalized();
}

inline cadm::vec3 CameraComponent::right() const
{
    return forward().cross(m_worldUp).normalized();
}

inline cadm::vec3 CameraComponent::up() const
{
    return right().cross(forward()).normalized();
}

#endif //CAD_CAMERA_HPP
