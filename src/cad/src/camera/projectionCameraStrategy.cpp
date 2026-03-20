//
// Created on 3/19/26.
//

#include "projectionCameraStrategy.hpp"

#include "../checkMacros.hpp"
#include "../components/transform.h"
#include <algorithm>
#include <numbers>

projectionCameraStrategy::projectionCameraStrategy(
    entity *cameraEntity,
    const std::function<int()> &widthGetter,
    const std::function<int()> &heightGetter)
    : ICameraStrategy(cameraEntity), m_widthGetter(widthGetter), m_heightGetter(heightGetter)
{
}

cadm::mat4 projectionCameraStrategy::getView()
{
    const auto camera = m_cameraEntity->getComponent<CameraComponent>();
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
    const auto camera = m_cameraEntity->getComponent<CameraComponent>();
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
    if (!m_mousePressed)
        return false;

    const auto camera = m_cameraEntity->getComponent<CameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }

    const auto pCamera = camera.value();


    const auto newAzimuth = pCamera->getAzimuthAngle() - static_cast<cadm::cadf>(mouseDelta.x()) * s_sensitivity;
    const auto newPolar = pCamera->getPolarAngle() - static_cast<cadm::cadf>(mouseDelta.y()) * s_sensitivity;

    pCamera->setAzimuthAngle(newAzimuth);
    pCamera->setPolarAngle(newPolar);

    return true;
}

bool projectionCameraStrategy::handleMousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        m_mousePressed = true;
    }
    return false;
}

bool projectionCameraStrategy::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        m_mousePressed = false;
    }
    return false;
}

bool projectionCameraStrategy::handleKeyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_W:
    case Qt::UpArrow:
        {
            const auto camera = m_cameraEntity->getComponent<CameraComponent>();
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
    case Qt::Key_S:
    case Qt::DownArrow:
        {
            const auto camera = m_cameraEntity->getComponent<CameraComponent>();
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
    case Qt::Key_A:
    case Qt::LeftArrow:
        {
            const auto camera = m_cameraEntity->getComponent<CameraComponent>();
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
    case Qt::Key_D:
    case Qt::RightArrow:
        {
            const auto camera = m_cameraEntity->getComponent<CameraComponent>();
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

    const auto camera = m_cameraEntity->getComponent<CameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }
    const auto pCamera = camera.value();

    auto newRadius = pCamera->getRadius();
    if (delta > 0)
    {
        newRadius /= m_zoomFactor;
    }
    else
    {
        newRadius *= m_zoomFactor;
    }

    pCamera->setRadius(newRadius);

    return true;
}

void projectionCameraStrategy::setZoomFactor(const cadm::cadf zoomFactor)
{
    m_zoomFactor = zoomFactor;
}
