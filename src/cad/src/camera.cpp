//
// Created by rdkgsk on 6/1/26.
//

#include "camera.h"

camera::camera(const cadm::vec3 &position, const cadm::vec3 &target, const cadm::vec3 &up)
    : m_position(position), m_target(target), m_up(up)
{
}

cadm::mat4 camera::getViewMatrix() const
{
    return cadm::mat4::lookAtRH(m_position, m_target, m_up);
}

cadm::mat4 camera::getProjectionMatrix() const
{
    // constexpr double height = 2.0;
    // const double width = height * m_aspectRatio;
    //
    // return cadm::mat4::ortho(-width / 2.0, width / 2.0, -height / 2.0, height / 2.0, m_nearPlane, m_farPlane);
    return cadm::mat4::projection(m_aspectRatio, m_fov, m_nearPlane, m_farPlane);
}

void camera::setPosition(const cadm::vec3 &position)
{
    m_position = position;
}

void camera::setTarget(const cadm::vec3 &target)
{
    m_target = target;
}

void camera::setUp(const cadm::vec3 &up)
{
    m_up = up;
}

void camera::setFov(const cadm::cadf fov)
{
    m_fov = fov;
}

void camera::setAspectRatio(const cadm::cadf aspectRatio)
{
    m_aspectRatio = aspectRatio;
}

void camera::setNearPlane(const cadm::cadf nearPlane)
{
    m_nearPlane = nearPlane;
}

void camera::setFarPlane(const cadm::cadf farPlane)
{
    m_farPlane = farPlane;
}