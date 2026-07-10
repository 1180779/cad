//
// Created on 3/19/26.
//

#include "BlenderCameraComponent.hpp"

cadm::Vec3 BlenderCameraComponent::forward() const {
    // camera looks along -Z in local space; orbitRot col2 is +Z; forward = -Z = -col2
    return -m_orbitRot.columns[2];
}

cadm::Vec3 BlenderCameraComponent::right() const {
    return m_orbitRot.columns[0];
}

cadm::Vec3 BlenderCameraComponent::up() const {
    return m_orbitRot.columns[1];
}

cadm::Vec3 BlenderCameraComponent::getPosition() const {
    return m_target + m_orbitRot.columns[2] * m_radius;
}

void BlenderCameraComponent::setTarget(const cadm::Vec3 &value) {
    if (m_target != value) {
        m_target = value;
        emit targetXChanged(value.x);
        emit targetYChanged(value.y);
        emit targetZChanged(value.z);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setTargetX(const cadm::cadf value) {
    if (std::abs(m_target.x - value) >= cadm::gc_eps) {
        m_target.x = value;
        emit targetXChanged(value);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setTargetY(const cadm::cadf value) {
    if (std::abs(m_target.y - value) >= cadm::gc_eps) {
        m_target.y = value;
        emit targetYChanged(value);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setTargetZ(const cadm::cadf value) {
    if (std::abs(m_target.z - value) >= cadm::gc_eps) {
        m_target.z = value;
        emit targetZChanged(value);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setRadius(cadm::cadf value) {
    value = std::max(value, s_minDistance);
    if (std::abs(m_radius - value) >= cadm::gc_eps) {
        m_radius = value;
        emit radiusChanged(value);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setFov(const cadm::cadf value) {
    if (const auto clampedValue = std::clamp(value, s_fovMin, s_fovMax);
        std::abs(m_fov - clampedValue) >= cadm::gc_eps) {
        m_fov = clampedValue;
        emit fovChanged(clampedValue);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setNearPlane(const cadm::cadf value) {
    auto clampedValue = std::clamp(value, s_nearPlaneMin, s_nearPlaneMax);
    clampedValue = std::min(clampedValue, m_farPlane - 0.01f);

    if (std::abs(m_nearPlane - clampedValue) >= cadm::gc_eps) {
        m_nearPlane = clampedValue;
        emit nearPlaneChanged(clampedValue);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setFarPlane(const cadm::cadf value) {
    auto clampedValue = std::clamp(value, s_farPlaneMin, s_farPlaneMax);
    clampedValue = std::max(clampedValue, m_nearPlane + 0.01f);

    if (std::abs(m_farPlane - clampedValue) >= cadm::gc_eps) {
        m_farPlane = clampedValue;
        emit farPlaneChanged(clampedValue);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setZoomFactor(const cadm::cadf factor) {
    if (const auto clampedValue = std::clamp(factor, s_zoomFactorMin, s_zoomFactorMax);
        std::abs(m_zoomFactor - clampedValue) >= cadm::gc_eps) {
        m_zoomFactor = clampedValue;
        emit zoomFactorChanged(clampedValue);
    }
}

void BlenderCameraComponent::setIsOrtho(const bool value) {
    if (m_isOrtho != value) {
        m_isOrtho = value;
        emit isOrthoChanged(value);
        emit propertyUpdated();
    }
}

void BlenderCameraComponent::setOrthoHeight(const cadm::cadf value) {
    if (const auto clampedValue = std::clamp(value, s_orthoHeightMin, s_orthoHeightMax);
        std::abs(m_orthoHeight - clampedValue) >= cadm::gc_eps) {
        m_orthoHeight = clampedValue;
        emit orthoHeightChanged(clampedValue);
        emit propertyUpdated();
    }
}
