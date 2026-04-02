//
// Created on 3/15/26.
//

#include "TransformComponent.hpp"

TransformComponent::TransformComponent() = default;

TransformComponent::TransformComponent(const cadm::vec3 &translation)
    : m_translation(translation)
{
}

TransformComponent::TransformComponent(const cadm::vec3 &translation, const cadm::vec3 &rotation)
    : m_translation(translation), m_rotation(rotation)
{
}

const cadm::mat4& TransformComponent::getModelMatrix() const
{
    if (m_isDirty)
    {
        m_isDirty = false;
        m_modelMatrix = cadm::mat4::translation(m_translation)
            * cadm::mat4::rotZYX(m_rotation)
            * cadm::mat4::scale(m_scale);
    }
    return m_modelMatrix;
}

void TransformComponent::setTranslation(const cadm::vec3 &translation)
{
    if (m_translation == translation) return;
    m_translation = translation;
    m_isDirty = true;
    emit translationXChanged(m_translation.x);
    emit translationYChanged(m_translation.y);
    emit translationZChanged(m_translation.z);
}

void TransformComponent::setScale(const cadm::vec3 &scale)
{
    if (m_scale == scale) return;
    m_scale = scale;
    m_isDirty = true;
    emit scaleXChanged(m_scale.x);
    emit scaleYChanged(m_scale.y);
    emit scaleZChanged(m_scale.z);
}

void TransformComponent::setRotation(const cadm::vec3 &rotation)
{
    if (m_rotation == rotation) return;
    m_rotation = rotation;
    m_isDirty = true;
    emit rotationXChanged(m_rotation.x);
    emit rotationYChanged(m_rotation.y);
    emit rotationZChanged(m_rotation.z);
}
