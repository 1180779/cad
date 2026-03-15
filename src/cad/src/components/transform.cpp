//
// Created on 3/15/26.
//

#include "transform.h"

const cadm::mat4& TransformComponent::getModelMatrix() const
{
    if (m_isDirty)
    {
        m_isDirty = false;

        m_modelMatrix = cadm::mat4::translation(m_translation) * cadm::mat4::rotZ(m_rotation.z) * cadm::mat4::rotY(
            m_rotation.y) * cadm::mat4::rotX(m_rotation.x) * cadm::mat4::scale(m_scale);
    }
    return m_modelMatrix;
}

void TransformComponent::setTranslation(const cadm::vec3 &translation)
{
    if (m_translation == translation) return;
    m_translation = translation;
    m_isDirty = true;
}

void TransformComponent::setScale(const cadm::vec3 &scale)
{
    if (m_scale == scale) return;
    m_scale = scale;
    m_isDirty = true;
}

void TransformComponent::setRotation(const cadm::vec3 &rotation)
{
    if (m_rotation == rotation) return;
    m_rotation = rotation;
    m_isDirty = true;
}
