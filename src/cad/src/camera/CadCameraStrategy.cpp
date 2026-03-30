//
// Created on 3/23/26.
//

#include "CadCameraStrategy.hpp"

#include "../CheckMacros.hpp"
#include "../components/CadCameraComponent.hpp"
#include "../components/ProjectionCameraComponent.hpp"
#include "../components/TransformComponent.hpp"

CadCameraStrategy::CadCameraStrategy(
    Entity *cameraEntity,
    const std::function<int()> &widthGetter,
    const std::function<int()> &heightGetter)
    : ICameraStrategy(cameraEntity), m_widthGetter{widthGetter}, m_heightGetter{heightGetter}
{
}

cadm::mat4 CadCameraStrategy::getView()
{
    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();
    const auto view = cadm::mat4::lookAtRH(pCamera->getPosition(), pCamera->getTarget(), pCamera->up());
    return view;
}

cadm::mat4 CadCameraStrategy::getProjection()
{
    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();
    const auto height = pCamera->getOrthoHeight();
    const auto width = height * pCamera->getAspectRatio();

    const auto projection = cadm::mat4::ortho(
        -width / 2.0,
        width / 2.0,
        -height / 2.0,
        height / 2.0,
        pCamera->getNearPlane(),
        pCamera->getFarPlane());
    return projection;
}

bool CadCameraStrategy::handleMouseMoveEvent(
    QMouseEvent *event,
    QPoint mouseDelta)
{
    if (!m_leftMouseDown && !m_rightMouseDown)
        return false;

    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera.has_value())
    {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }

    const auto pCamera = camera.value();
    if (m_leftMouseDown)
    {
        const auto yawAngle = -static_cast<cadm::cadf>(mouseDelta.x()) * pCamera->getRotationSpeed();
        const auto pitchAngle = static_cast<cadm::cadf>(mouseDelta.y()) * pCamera->getRotationSpeed();

        const auto yawRot = cadm::mat4::rotAxis(yawAngle, pCamera->getWorldUp()).upperLeft3x3();
        const auto pitchRot = cadm::mat4::rotAxis(pitchAngle, pCamera->right()).upperLeft3x3();
        const auto combinedRot = yawRot * pitchRot;

        // TODO: choose pivot based on ray from camera
        constexpr auto pivot = cadm::vec3{};
        const auto relTarget = pCamera->getTarget() - pivot;
        const auto relPosition = pCamera->getPosition() - pivot;
        auto newRelPos = combinedRot * relPosition;
        auto newRelTarget = combinedRot * relTarget;

        newRelPos = newRelPos.safeNormalized(cadm::vec3::unitZ()) * relPosition.length();
        newRelTarget = newRelTarget.safeNormalized(cadm::vec3{}) * relTarget.length();

        const auto newPosition = pivot + newRelPos;
        const auto newTarget = pivot + newRelTarget;

        const auto rotatedUp = combinedRot * pCamera->up();
        const auto newForward = (newTarget - newPosition).safeNormalized(cadm::vec3::unitZ());
        const auto newUp = (rotatedUp - newForward * rotatedUp.dot(newForward))
            .safeNormalized(pCamera->getWorldUp());

        pCamera->setPosition(newPosition);
        pCamera->setTarget(newTarget);
        pCamera->setUp(newUp);

        if (const auto transform = m_cameraEntity->getComponent<TransformComponent>();
            transform.has_value())
        {
            transform.value()->setTranslation(newPosition);
        }
    }

    if (m_rightMouseDown)
    {
        const cadm::cadf changeX = -pCamera->getAspectRatio() * pCamera->getOrthoHeight() * static_cast<cadm::cadf>(
            mouseDelta.x()) / static_cast<cadm::cadf>(m_widthGetter());
        const cadm::cadf changeY = pCamera->getOrthoHeight() * static_cast<cadm::cadf>(mouseDelta.y()) / static_cast<
            cadm::cadf>(m_heightGetter());
        const auto translationChange = pCamera->right() * changeX
            + pCamera->up() * changeY;
        const auto newPosition = pCamera->getPosition() + translationChange;
        const auto newTarget = pCamera->getTarget() + translationChange;
        pCamera->setPosition(newPosition);
        pCamera->setTarget(newTarget);
    }

    return true;
}

bool CadCameraStrategy::handleMousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_leftMouseDown = true;
    else if (event->button() == Qt::RightButton)
        m_rightMouseDown = true;
    return false;
}

bool CadCameraStrategy::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_leftMouseDown = false;
    else if (event->button() == Qt::RightButton)
        m_rightMouseDown = false;
    return false;
}

bool CadCameraStrategy::handleKeyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case s_keyDown:
        {
            const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->getOrthoHeight();
            const auto up = pCamera->up();
            const auto position = pCamera->getPosition() + up * step;
            const auto target = pCamera->getTarget() + up * step;
            pCamera->setPosition(position);
            pCamera->setTarget(target);
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(position);
            }
            return true;
        }
    case s_keyUp:
        {
            const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->getOrthoHeight();
            const auto up = pCamera->up();
            const auto position = pCamera->getPosition() - up * step;
            const auto target = pCamera->getTarget() - up * step;
            pCamera->setPosition(position);
            pCamera->setTarget(target);
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(position);
            }
            return true;
        }
    case s_keyLeft:
        {
            const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->getOrthoHeight();
            const auto right = pCamera->right();
            const auto position = pCamera->getPosition() - right * step;
            const auto target = pCamera->getTarget() - right * step;
            pCamera->setPosition(position);
            pCamera->setTarget(target);
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(position);
            }
            return true;
        }
    case s_keyRight:
        {
            const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->getOrthoHeight();
            const auto right = pCamera->right();
            const auto position = pCamera->getPosition() + right * step;
            const auto target = pCamera->getTarget() + right * step;
            pCamera->setPosition(position);
            pCamera->setTarget(target);
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(position);
            }
            return true;
        }
    default: return false;
    }
    return false;
}

bool CadCameraStrategy::handleWheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    if (delta == 0)
        return false;

    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }
    const auto pCamera = camera.value();

    const auto pos = event->position();
    const auto nx = pos.x() / m_widthGetter() * 2.0 - 1.0;
    const auto ny = 1.0 - pos.y() / m_heightGetter() * 2.0;

    const auto oldHeight = pCamera->getOrthoHeight();
    const auto oldWidth = oldHeight * pCamera->getAspectRatio();

    if (delta > 0)
    {
        const auto newOrthoHeight = pCamera->getOrthoHeight() / pCamera->getZoomFactor();
        pCamera->setOrthoHeight(newOrthoHeight);
    }
    else
    {
        const auto newOrthoHeight = pCamera->getOrthoHeight() * pCamera->getZoomFactor();
        pCamera->setOrthoHeight(newOrthoHeight);
    }

    const auto newHeight = pCamera->getOrthoHeight();
    const auto newWidth = newHeight * pCamera->getAspectRatio();

    const auto deltaX = static_cast<cadm::cadf>((oldWidth - newWidth) / 2.0 * nx);
    const auto deltaY = static_cast<cadm::cadf>((oldHeight - newHeight) / 2.0 * ny);

    const auto right = pCamera->right();
    const auto up = pCamera->up();

    const auto translation = right * deltaX + up * deltaY;
    const auto position = pCamera->getPosition() + translation;
    const auto target = pCamera->getTarget() + translation;
    pCamera->setPosition(position);
    pCamera->setTarget(target);

    if (const auto transform = m_cameraEntity->getComponent<TransformComponent>())
    {
        transform.value()->setTranslation(position);
    }
    return true;
}
