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
    explicit CameraComponent(const cadm::vec3 &up)
        : m_up(up)
    {
    }

    cadm::vec3 m_up{};

    cadm::cadf m_fov{M_PI / 3}; // 60 degrees
    cadm::cadf m_aspectRatio{1.0f};
    cadm::cadf m_nearPlane{0.1f};
    cadm::cadf m_farPlane{100.0f};
};

#endif //CAD_CAMERA_HPP