//
// Created on 3/23/26.
//

#include "cadCameraStrategy.hpp"

#include "../checkMacros.hpp"
#include "../components/cadCameraCompoonent.hpp"
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
    const auto camera = m_cameraEntity->getComponent<cadCameraComponent>();
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
    const auto camera = m_cameraEntity->getComponent<cadCameraComponent>();
    if (!camera)
    {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();
    const double height = pCamera->getOrthoHeight();
    const double width = height * pCamera->getAspectRatio();

    const auto projection = cadm::mat4::ortho(
        -width / 2.0,
        width / 2.0,
        -height / 2.0,
        height / 2.0,
        pCamera->getNearPlane(),
        pCamera->getFarPlane());
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
            const auto camera = m_cameraEntity->getComponent<cadCameraComponent>();
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
    case Qt::Key_S:
    case Qt::DownArrow:
        {
            const auto camera = m_cameraEntity->getComponent<cadCameraComponent>();
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
    case Qt::Key_A:
    case Qt::LeftArrow:
        {
            const auto camera = m_cameraEntity->getComponent<cadCameraComponent>();
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
    case Qt::Key_D:
    case Qt::RightArrow:
        {
            const auto camera = m_cameraEntity->getComponent<cadCameraComponent>();
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

    const auto camera = m_cameraEntity->getComponent<cadCameraComponent>();
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
        const auto newOrthoHeight = pCamera->getOrthoHeight() / m_zoomFactor;
        pCamera->setOrthoHeight(newOrthoHeight);
    }
    else
    {
        const auto newOrthoHeight = pCamera->getOrthoHeight() * m_zoomFactor;
        pCamera->setOrthoHeight(newOrthoHeight);
    }

    const auto newHeight = pCamera->getOrthoHeight();
    const auto newWidth = newHeight * pCamera->getAspectRatio();

    const auto deltaX = (oldWidth - newWidth) / 2.0 * nx;
    const auto deltaY = (oldHeight - newHeight) / 2.0 * ny;

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

void CadCameraStrategy::setZoomFactor(cadm::cadf zoomFactor)
{
    m_zoomFactor = zoomFactor;
}
