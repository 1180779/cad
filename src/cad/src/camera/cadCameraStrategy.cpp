//
// Created on 3/18/26.
//

#include "cadCameraStrategy.hpp"

#include "../checkMacros.hpp"
#include "../components/camera.hpp"
#include "../components/transform.h"

CadCameraStrategy::CadCameraStrategy(
    entity *cameraEntity,
    const std::function<int()> &widthGetter,
    const std::function<int()> &heightGetter)
    : ICameraStrategy(cameraEntity), m_widthGetter{widthGetter}, m_heightGetter{heightGetter}
{
}

cadm::mat4 CadCameraStrategy::getView()
{
    const auto camera = m_cameraEntity->getComponent<CameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();
    const auto view = cadm::mat4::lookAtRH(pCamera->m_position, pCamera->m_target, pCamera->up());
    return view;
}

cadm::mat4 CadCameraStrategy::getProjection()
{
    const auto camera = m_cameraEntity->getComponent<CameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();
    const double height = pCamera->m_orthoHeight;
    const double width = height * pCamera->m_aspectRatio;

    const auto projection = cadm::mat4::ortho(
        -width / 2.0,
        width / 2.0,
        -height / 2.0,
        height / 2.0,
        pCamera->m_nearPlane,
        pCamera->m_farPlane);
    return projection;
}

bool CadCameraStrategy::handleMouseMoveEvent(QMouseEvent *event, QPoint mouseDelta)
{
    return false;
}

bool CadCameraStrategy::handleMousePressEvent(QMouseEvent *event)
{
    return false;
}

bool CadCameraStrategy::handleMouseReleaseEvent(QMouseEvent *event)
{
    return false;
}

bool CadCameraStrategy::handleKeyPressEvent(QKeyEvent *event)
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
            const auto step = m_translationStep * pCamera->m_orthoHeight;
            const auto up = pCamera->up();
            pCamera->m_position += up * step;
            pCamera->m_target += up * step;
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->m_position);
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
            const auto step = m_translationStep * pCamera->m_orthoHeight;
            const auto up = pCamera->up();
            pCamera->m_position -= up * step;
            pCamera->m_target -= up * step;
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->m_position);
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
            const auto step = m_translationStep * pCamera->m_orthoHeight;
            const auto right = pCamera->right();
            pCamera->m_position -= right * step;
            pCamera->m_target -= right * step;
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->m_position);
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
            const auto step = m_translationStep * pCamera->m_orthoHeight;
            const auto right = pCamera->right();
            pCamera->m_position += right * step;
            pCamera->m_target += right * step;
            if (auto transform = m_cameraEntity->getComponent<TransformComponent>())
            {
                transform.value()->setTranslation(pCamera->m_position);
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

    const auto camera = m_cameraEntity->getComponent<CameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }
    const auto pCamera = camera.value();

    const auto pos = event->position();
    const auto nx = pos.x() / m_widthGetter() * 2.0 - 1.0;
    const auto ny = 1.0 - pos.y() / m_heightGetter() * 2.0;

    const auto oldHeight = pCamera->m_orthoHeight;
    const auto oldWidth = oldHeight * pCamera->m_aspectRatio;

    if (delta > 0)
    {
        pCamera->m_orthoHeight /= m_zoomFactor;
    }
    else
    {
        pCamera->m_orthoHeight *= m_zoomFactor;
    }

    const auto newHeight = pCamera->m_orthoHeight;
    const auto newWidth = newHeight * pCamera->m_aspectRatio;

    const auto deltaX = (oldWidth - newWidth) / 2.0 * nx;
    const auto deltaY = (oldHeight - newHeight) / 2.0 * ny;

    const auto right = pCamera->right();
    const auto up = pCamera->up();

    const auto translation = right * deltaX + up * deltaY;
    pCamera->m_position += translation;
    pCamera->m_target += translation;

    if (const auto transform = m_cameraEntity->getComponent<TransformComponent>())
    {
        transform.value()->setTranslation(pCamera->m_position);
    }
    return true;
}

void CadCameraStrategy::setZoomFactor(cadm::cadf zoomFactor)
{
    m_zoomFactor = zoomFactor;
}
