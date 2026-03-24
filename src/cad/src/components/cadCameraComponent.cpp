//
// Created on 3/23/26.
//

#include "cadCameraCompoonent.hpp"

cadm::vec3 cadCameraComponent::forward() const
{
    return (m_target - m_position).normalized();
}

cadm::vec3 cadCameraComponent::right() const
{
    return forward().cross(m_worldUp).normalized();
}

cadm::vec3 cadCameraComponent::up() const
{
    return right().cross(forward()).normalized();
}

void cadCameraComponent::setPosition(const cadm::vec3 &position)
{
    if (m_position != position)
    {
        m_position = position;
        emit positionXChanged(position.x);
        emit positionYChanged(position.y);
        emit positionZChanged(position.z);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setPositionX(const cadm::cadf x)
{
    if (std::abs(m_position.x - x) >= cadm::eps)
    {
        m_position.x = x;
        emit positionXChanged(x);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setPositionY(const cadm::cadf y)
{
    if (std::abs(m_position.y - y) >= cadm::eps)
    {
        m_position.y = y;
        emit positionYChanged(y);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setPositionZ(const cadm::cadf z)
{
    if (std::abs(m_position.z - z) >= cadm::eps)
    {
        m_position.z = z;
        emit positionZChanged(z);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setTarget(const cadm::vec3 &target)
{
    if (m_target != target)
    {
        m_target = target;
        emit targetXChanged(target.x);
        emit targetYChanged(target.y);
        emit targetZChanged(target.z);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setTargetX(const cadm::cadf x)
{
    if (std::abs(m_target.x - x) >= cadm::eps)
    {
        m_target.x = x;
        emit targetXChanged(x);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setTargetY(const cadm::cadf y)
{
    if (std::abs(m_target.y - y) >= cadm::eps)
    {
        m_target.y = y;
        emit targetYChanged(y);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setTargetZ(const cadm::cadf z)
{
    if (std::abs(m_target.z - z) >= cadm::eps)
    {
        m_target.z = z;
        emit targetZChanged(z);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setWorldUp(const cadm::vec3 &worldUp)
{
    if (m_worldUp != worldUp)
    {
        m_worldUp = worldUp;
        emit worldUpXChanged(worldUp.x);
        emit worldUpYChanged(worldUp.y);
        emit worldUpZChanged(worldUp.z);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setWorldUpX(const cadm::cadf x)
{
    if (std::abs(m_worldUp.x - x) >= cadm::eps)
    {
        m_worldUp.x = x;
        emit worldUpXChanged(x);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setWorldUpY(const cadm::cadf y)
{
    if (std::abs(m_worldUp.y - y) >= cadm::eps)
    {
        m_worldUp.y = y;
        emit worldUpYChanged(y);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setWorldUpZ(const cadm::cadf z)
{
    if (std::abs(m_worldUp.z - z) >= cadm::eps)
    {
        m_worldUp.z = z;
        emit worldUpZChanged(z);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setNearPlane(const cadm::cadf nearPlane)
{
    auto clampedValue = std::clamp(nearPlane, s_nearPlaneMin, s_nearPlaneMax);
    clampedValue = std::min(clampedValue, m_farPlane - 0.01f);

    if (std::abs(m_nearPlane - clampedValue) >= cadm::eps)
    {
        m_nearPlane = clampedValue;
        emit nearPlaneChanged(clampedValue);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setFarPlane(const cadm::cadf farPlane)
{
    auto clampedValue = std::clamp(farPlane, s_farPlaneMin, s_farPlaneMax);
    clampedValue = std::max(clampedValue, m_nearPlane + 0.01f);

    if (std::abs(m_farPlane - clampedValue) >= cadm::eps)
    {
        m_farPlane = clampedValue;
        emit farPlaneChanged(clampedValue);
        emit propertyUpdated();
    }
}

void cadCameraComponent::setOrthoHeight(const cadm::cadf height)
{
    if (const auto clampedValue = std::clamp(height, s_orthoHeightMin, s_orthoHeightMax);
        std::abs(m_orthoHeight - clampedValue) >= cadm::eps)
    {
        m_orthoHeight = clampedValue;
        emit orthoHeightChanged(clampedValue);
        emit propertyUpdated();
    }
}
