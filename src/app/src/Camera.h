//
// Created by rdkgsk on 6/1/26.
//

#ifndef CAD_CAMERA_H
#define CAD_CAMERA_H

#define _USE_MATH_DEFINES
#include <cmath>

#include <cad_math/vec3.h>
#include <cad_math/mat4.h>


class Camera
{
public:
    Camera(cadm::vec3 position, cadm::vec3 target, cadm::vec3 up);

    [[nodiscard]] cadm::mat4 getViewMatrix() const;
    [[nodiscard]] cadm::mat4 getProjectionMatrix() const;

    void setPosition(cadm::vec3 position);
    void setTarget(cadm::vec3 target);
    void setUp(cadm::vec3 up);

    void setFov(cadm::cadf fov);
    void setAspectRatio(cadm::cadf aspectRatio);
    void setNearPlane(cadm::cadf nearPlane);
    void setFarPlane(cadm::cadf farPlane);

    [[nodiscard]] cadm::vec3 getPosition() const { return m_position; }
    [[nodiscard]] cadm::vec3 getTarget() const { return m_target; }
    [[nodiscard]] cadm::vec3 getUp() const { return m_up; }

    [[nodiscard]] cadm::cadf getFov() const { return m_fov; }
    [[nodiscard]] cadm::cadf getAspectRatio() const { return m_aspectRatio; }
    [[nodiscard]] cadm::cadf getNearPlane() const { return m_nearPlane; }
    [[nodiscard]] cadm::cadf getFarPlane() const { return m_farPlane; }

    [[nodiscard]] cadm::vec3 getForwardVector() const
    {
        return (m_target - m_position).normalized();
    }

private:
    cadm::vec3 m_position;
    cadm::vec3 m_target;
    cadm::vec3 m_up;

    cadm::cadf m_fov{M_PI / 3}; // 60 degrees
    cadm::cadf m_aspectRatio{1.0};
    cadm::cadf m_nearPlane{0.1};
    cadm::cadf m_farPlane{100.0};
};

#endif //CAD_CAMERA_H
