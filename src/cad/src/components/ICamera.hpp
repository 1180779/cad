//
// Created on 3/23/26.
//

#ifndef CAD_ICAMERA_HPP
#define CAD_ICAMERA_HPP

#include "../entities/entity.h"
#include <cad_math/common.h>

class CameraComponent : public Component
{
public:
    ~CameraComponent() override = default;

    [[nodiscard]] cadm::cadf getAspectRatio() const { return m_aspectRatio; }
    void setAspectRatio(cadm::cadf value);

    static constexpr cadm::cadf s_nearPlaneMin = 0.01;
    static constexpr cadm::cadf s_nearPlaneMax = 10000.0;

    static constexpr cadm::cadf s_farPlaneMin = 0.01;
    static constexpr cadm::cadf s_farPlaneMax = 10000.0;

protected:
    cadm::cadf m_aspectRatio{1.0f};
};

inline void CameraComponent::setAspectRatio(const cadm::cadf value)
{
    if (std::abs(m_aspectRatio - value) >= cadm::eps)
    {
        m_aspectRatio = value;
    }
}
#endif //CAD_ICAMERA_HPP