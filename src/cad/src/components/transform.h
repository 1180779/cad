//
// Created on 3/15/26.
//

#ifndef CAD_TRANSFORM_H
#define CAD_TRANSFORM_H

#include "../entities/entity.h"
#include <cad_math/vec3.h>
#include <cad_math/mat4.h>

class TransformComponent : public Component {
public:
    [[nodiscard]] const cadm::mat4& getModelMatrix() const;
    [[nodiscard]] cadm::vec3 getTranslation() const {return m_translation;}
    [[nodiscard]] cadm::vec3 getScale() const {return m_scale;}
    [[nodiscard]] cadm::vec3 getRotation() const {return m_rotation;}

    void setTranslation(const cadm::vec3& translation);
    void setScale(const cadm::vec3& scale);
    void setRotation(const cadm::vec3& rotation);

private:
    cadm::vec3 m_translation{0.0f, 0.0f, 0.0f};

    // Euler angles for now
    // TODO: replace with quaternions
    cadm::vec3 m_rotation{0.0f, 0.0f, 0.0f};
    cadm::vec3 m_scale{1.0f, 1.0f, 1.0f};

    mutable cadm::mat4 m_modelMatrix = cadm::mat4::identity();
    mutable bool m_isDirty = true;
};

#endif //CAD_TRANSFORM_H