//
// Created on 3/19/26.
//

#include "camera.hpp"

cadm::vec3 CameraComponent::forward() const
{
    return (m_target - getPosition()).normalized();
}

cadm::vec3 CameraComponent::right() const
{
    return forward().cross(m_worldUp).normalized();
}

cadm::vec3 CameraComponent::up() const
{
    return right().cross(forward()).normalized();
}

cadm::vec3 CameraComponent::getPosition() const
{
    const auto sinPolar = std::sin(m_polarAngle);
    const auto cosPolar = std::cos(m_polarAngle);
    const auto sinAzimuth = std::sin(m_azimuthAngle);
    const auto cosAzimuth = std::cos(m_azimuthAngle);

    return cadm::vec3{
        m_target.x + m_radius * cosPolar * cosAzimuth,
        m_target.y + m_radius * sinPolar,
        m_target.z + m_radius * cosPolar * sinAzimuth
    };
}

void CameraComponent::setTarget(const cadm::vec3 &value)
{
    if (m_target != value)
    {
        m_target = value;
        emit targetXChanged(value.x);
        emit targetYChanged(value.y);
        emit targetZChanged(value.z);
        emit propertyUpdated();
    }
}

void CameraComponent::setTargetX(const cadm::cadf value)
{
    if (std::abs(m_target.x - value) >= cadm::eps)
    {
        m_target.x = value;
        emit targetXChanged(value);
        emit propertyUpdated();
    }
}

void CameraComponent::setTargetY(const cadm::cadf value)
{
    if (std::abs(m_target.y - value) >= cadm::eps)
    {
        m_target.y = value;
        emit targetYChanged(value);
        emit propertyUpdated();
    }
}

void CameraComponent::setTargetZ(const cadm::cadf value)
{
    if (std::abs(m_target.z - value) >= cadm::eps)
    {
        m_target.z = value;
        emit targetZChanged(value);
        emit propertyUpdated();
    }
}

void CameraComponent::setRadius(cadm::cadf value)
{
    value = std::max(value, s_minDistance);
    if (std::abs(m_radius - value) >= cadm::eps)
    {
        m_radius = value;
        emit radiusChanged(value);
        emit propertyUpdated();
    }
}

void CameraComponent::setAzimuthAngle(const cadm::cadf value)
{
    constexpr auto twoPI = s_azimuthAngleMax - s_azimuthAngleMin;
    cadm::cadf wrappedAngle = std::fmod(value - s_azimuthAngleMin, twoPI);
    if (wrappedAngle < 0)
    {
        wrappedAngle += twoPI;
    }
    wrappedAngle += s_azimuthAngleMin;

    if (std::abs(m_azimuthAngle - wrappedAngle) >= cadm::eps)
    {
        m_azimuthAngle = wrappedAngle;
        emit azimuthAngleChanged(wrappedAngle);
        emit propertyUpdated();
    }
}

void CameraComponent::setPolarAngle(const cadm::cadf value)
{
    if (const auto clampedAngle = std::clamp(value, s_polarAngleMin, s_polarAngleMax); std::abs(
        m_polarAngle - clampedAngle) >= cadm::eps)
    {
        m_polarAngle = clampedAngle;
        emit polarAngleChanged(clampedAngle);
        emit propertyUpdated();
    }
}

void CameraComponent::setFov(const cadm::cadf value)
{
    if (const auto clampedValue = std::clamp(value, s_fovMin, s_fovMax); std::abs(m_fov - clampedValue) >= cadm::eps)
    {
        m_fov = clampedValue;
        emit fovChanged(clampedValue);
        emit propertyUpdated();
    }
}

void CameraComponent::setNearPlane(const cadm::cadf value)
{
    auto clampedValue = std::clamp(value, s_nearPlaneMin, s_nearPlaneMax);
    clampedValue = std::min(clampedValue, m_farPlane - 0.01f);

    if (std::abs(m_nearPlane - clampedValue) >= cadm::eps)
    {
        m_nearPlane = clampedValue;
        emit nearPlaneChanged(clampedValue);
        emit propertyUpdated();
    }
}

void CameraComponent::setFarPlane(const cadm::cadf value)
{
    auto clampedValue = std::clamp(value, s_farPlaneMin, s_farPlaneMax);
    clampedValue = std::max(clampedValue, m_nearPlane + 0.01f);

    if (std::abs(m_farPlane - clampedValue) >= cadm::eps)
    {
        m_farPlane = clampedValue;
        emit farPlaneChanged(clampedValue);
        emit propertyUpdated();
    }
}

void CameraComponent::setAspectRatio(const cadm::cadf value)
{
    if (std::abs(m_aspectRatio - value) >= cadm::eps)
    {
        m_aspectRatio = value;
        emit propertyUpdated();
    }
}
