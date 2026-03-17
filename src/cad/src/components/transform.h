//
// Created on 3/15/26.
//

#ifndef CAD_TRANSFORM_H
#define CAD_TRANSFORM_H

#include "../entities/entity.h"
#include <cad_math/vec3.h>
#include <cad_math/mat4.h>

class TransformComponent final : public Component
{
public:
    TransformComponent();
    explicit TransformComponent(const cadm::vec3 &translation);
    TransformComponent(const cadm::vec3 &translation, const cadm::vec3 &rotation);

    [[nodiscard]] const cadm::mat4& getModelMatrix() const;
    [[nodiscard]] cadm::vec3 getTranslation() const { return m_translation; }
    [[nodiscard]] cadm::vec3 getScale() const { return m_scale; }
    [[nodiscard]] cadm::vec3 getRotation() const { return m_rotation; }

    void setTranslation(const cadm::vec3 &translation);
    void setScale(const cadm::vec3 &scale);
    void setRotation(const cadm::vec3 &rotation);

private:
    cadm::vec3 m_translation{};

    // Euler angles for now
    // TODO: replace with quaternions
    cadm::vec3 m_rotation{};
    cadm::vec3 m_scale{1.0f, 1.0f, 1.0f};

    mutable cadm::mat4 m_modelMatrix = cadm::mat4::identity();
    mutable bool m_isDirty = true;
};

#endif //CAD_TRANSFORM_H
