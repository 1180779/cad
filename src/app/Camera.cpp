//
// Created by rdkgsk on 6/1/26.
//

#include "Camera.h"

Camera::Camera(cadm::vec3 position, cadm::vec3 target, cadm::vec3 up)
    : m_position(position), m_target(target), m_up(up)
{
}

cadm::mat4 Camera::getViewMatrix() const
{
    return cadm::mat4::lookAtRH(m_position, m_target, m_up);
}

cadm::mat4 Camera::getProjectionMatrix() const
{
    constexpr double height = 2.0;
    const double width = height * m_aspectRatio;

    return cadm::mat4::ortho(-width / 2.0, width / 2.0, -height / 2.0, height / 2.0, m_nearPlane, m_farPlane);
}

void Camera::setPosition(cadm::vec3 position)
{
    m_position = position;
}

void Camera::setTarget(cadm::vec3 target)
{
    m_target = target;
}

void Camera::setUp(cadm::vec3 up)
{
    m_up = up;
}

void Camera::setFov(cadm::cadf fov)
{
    m_fov = fov;
}

void Camera::setAspectRatio(cadm::cadf aspectRatio)
{
    m_aspectRatio = aspectRatio;
}

void Camera::setNearPlane(cadm::cadf nearPlane)
{
    m_nearPlane = nearPlane;
}

void Camera::setFarPlane(cadm::cadf farPlane)
{
    m_farPlane = farPlane;
}
