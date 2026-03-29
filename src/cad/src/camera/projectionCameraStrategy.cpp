//
// Created on 3/19/26.
//

#include "projectionCameraStrategy.hpp"

#include "../checkMacros.hpp"
#include "../components/camera.hpp"
#include "../components/transform.hpp"

projectionCameraStrategy::projectionCameraStrategy(
    entity *cameraEntity,
    const std::function<int()> &widthGetter,
    const std::function<int()> &heightGetter)
    : ICameraStrategy(cameraEntity), m_widthGetter(widthGetter), m_heightGetter(heightGetter)
{
}

cadm::mat4 projectionCameraStrategy::getView()
{
    const auto camera = m_cameraEntity->getComponent<ProjectionCameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();
    const auto view = cadm::mat4::lookAtRH(pCamera->getPosition(), pCamera->getTarget(), pCamera->up());
    return view;
}

cadm::mat4 projectionCameraStrategy::getProjection()
{
    const auto camera = m_cameraEntity->getComponent<ProjectionCameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();

    return cadm::mat4::projectionMO(
        pCamera->getAspectRatio(),
        pCamera->getFov(),
        pCamera->getNearPlane(),
        pCamera->getFarPlane()
    );
}

bool projectionCameraStrategy::handleMouseMoveEvent(QMouseEvent *event, const QPoint mouseDelta)
{
    if (!m_leftMouseDown && !m_rightMouseDown)
        return false;

    const auto camera = m_cameraEntity->getComponent<ProjectionCameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }

    const auto pCamera = camera.value();

    if (m_leftMouseDown)
    {
        const auto newAzimuth = pCamera->getAzimuthAngle() - static_cast<cadm::cadf>(mouseDelta.x()) * s_sensitivity;
        const auto newPolar = pCamera->getPolarAngle() - static_cast<cadm::cadf>(mouseDelta.y()) * s_sensitivity;

        pCamera->setAzimuthAngle(newAzimuth);
        pCamera->setPolarAngle(newPolar);
    }

    if (m_rightMouseDown)
    {
        const cadm::cadf scale = 2.0 * pCamera->getRadius()
            * std::tan(pCamera->getFov() / 2.0)
            / static_cast<cadm::cadf>(m_heightGetter());
        const auto translationChange = pCamera->right() * (-scale * static_cast<cadm::cadf>(mouseDelta.x()))
            + pCamera->up() * (scale * static_cast<cadm::cadf>(mouseDelta.y()));
        pCamera->setTarget(pCamera->getTarget() + translationChange);
    }

    return true;
}

bool projectionCameraStrategy::handleMousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        m_leftMouseDown = true;
    }
    else if (event->button() == Qt::MouseButton::RightButton)
    {
        m_rightMouseDown = true;
    }
    return false;
}

bool projectionCameraStrategy::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        m_leftMouseDown = false;
    }
    else if (event->button() == Qt::MouseButton::RightButton)
    {
        m_rightMouseDown = false;
    }
    return false;
}

bool projectionCameraStrategy::handleKeyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case s_keyUp:
        {
            const auto camera = m_cameraEntity->getComponent<ProjectionCameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->getRadius();
            const auto up = pCamera->up();
            const auto newTarget = pCamera->getTarget() + up * step;
            pCamera->setTarget(newTarget);
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->getPosition());
            }
            return true;
        }
    case s_keyDown:
        {
            const auto camera = m_cameraEntity->getComponent<ProjectionCameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->getRadius();
            const auto up = pCamera->up();
            const auto newTarget = pCamera->getTarget() - up * step;
            pCamera->setTarget(newTarget);
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->getPosition());
            }
            return true;
        }
    case s_keyLeft:
        {
            const auto camera = m_cameraEntity->getComponent<ProjectionCameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->getRadius();
            const auto right = pCamera->right();
            const auto newTarget = pCamera->getTarget() - right * step;
            pCamera->setTarget(newTarget);
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->getPosition());
            }
            return true;
        }
    case s_keyRight:
        {
            const auto camera = m_cameraEntity->getComponent<ProjectionCameraComponent>();
            if (!camera)
                break;
            const auto pCamera = camera.value();
            const auto step = m_translationStep * pCamera->getRadius();
            const auto right = pCamera->right();
            const auto newTarget = pCamera->getTarget() + right * step;
            pCamera->setTarget(newTarget);
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->getPosition());
            }
            return true;
        }
    default: return false;
    }
    return false;
}

bool projectionCameraStrategy::handleWheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    if (delta == 0)
        return false;

    const auto camera = m_cameraEntity->getComponent<ProjectionCameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }
    const auto pCamera = camera.value();

    auto newRadius = pCamera->getRadius();
    if (delta > 0)
    {
        newRadius /= pCamera->getZoomFactor();
    }
    else
    {
        newRadius *= pCamera->getZoomFactor();
    }

    pCamera->setRadius(newRadius);

    return true;
}
