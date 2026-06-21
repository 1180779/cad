//
// Created by rdkgsk on 6/1/26.
//

#ifndef CAD_CAMERA_H
#define CAD_CAMERA_H

#define _USE_MATH_DEFINES
#include <cmath>

#include <cad_math/Vec3.hpp>
#include <cad_math/Mat4.hpp>

class Camera {
public:
    Camera(const cadm::Vec3 &position, const cadm::Vec3 &target, const cadm::Vec3 &up);

    [[nodiscard]] cadm::Mat4 getViewMatrix() const;

    [[nodiscard]] cadm::Mat4 getProjectionMatrix() const;

    void setPosition(const cadm::Vec3 &position);

    void setTarget(const cadm::Vec3 &target);

    void setUp(const cadm::Vec3 &up);

    void setFov(cadm::cadf fov);

    void setAspectRatio(cadm::cadf aspectRatio);

    void setNearPlane(cadm::cadf nearPlane);

    void setFarPlane(cadm::cadf farPlane);

    [[nodiscard]] cadm::Vec3 getPosition() const {
        return m_position;
    }

    [[nodiscard]] cadm::Vec3 getTarget() const {
        return m_target;
    }

    [[nodiscard]] cadm::Vec3 getUp() const {
        return m_up;
    }

    [[nodiscard]] cadm::cadf getFov() const {
        return m_fov;
    }

    [[nodiscard]] cadm::cadf getAspectRatio() const {
        return m_aspectRatio;
    }

    [[nodiscard]] cadm::cadf getNearPlane() const {
        return m_nearPlane;
    }

    [[nodiscard]] cadm::cadf getFarPlane() const {
        return m_farPlane;
    }

    [[nodiscard]] cadm::Vec3 getForwardVector() const {
        return (m_target - m_position).normalized();
    }

private:
    cadm::Vec3 m_position;
    cadm::Vec3 m_target;
    cadm::Vec3 m_up;

    cadm::cadf m_fov{M_PI / 3}; // 60 degrees
    cadm::cadf m_aspectRatio{1.0};
    cadm::cadf m_nearPlane{0.1};
    cadm::cadf m_farPlane{100.0};
};

#endif //CAD_CAMERA_H