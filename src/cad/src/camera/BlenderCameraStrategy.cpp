//
// Created on 3/19/26.
//

#include "BlenderCameraStrategy.hpp"

#include "../CheckMacros.hpp"
#include "../components/BlenderCameraComponent.hpp"
#include "../components/TransformComponent.hpp"

BlenderCameraStrategy::BlenderCameraStrategy(
    Entity *cameraEntity,
    const std::function<int()> &widthGetter,
    const std::function<int()> &heightGetter
) : ICameraStrategy(cameraEntity, widthGetter, heightGetter) {}

cadm::Mat4 BlenderCameraStrategy::getView() {
    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return cadm::Mat4::identity();
    }
    const auto pCamera = camera.value();
    const auto view = cadm::Mat4::lookAtRh(pCamera->getPosition(), pCamera->getTarget(), pCamera->up());
    return view;
}

cadm::Mat4 BlenderCameraStrategy::getProjection() {
    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return cadm::Mat4::identity();
    }
    const auto pCamera = camera.value();

    if (pCamera->isOrtho()) {
        const auto h = pCamera->getOrthoHeight();
        const auto w = h * pCamera->getAspectRatio();
        return cadm::Mat4::ortho(
            static_cast<cadm::cadf>(-w / 2.0),
            static_cast<cadm::cadf>(w / 2.0),
            static_cast<cadm::cadf>(-h / 2.0),
            static_cast<cadm::cadf>(h / 2.0),
            pCamera->getNearPlane(),
            pCamera->getFarPlane()
        );
    }

    return cadm::Mat4::perspective(
        pCamera->getAspectRatio(),
        pCamera->getFov(),
        pCamera->getNearPlane(),
        pCamera->getFarPlane()
    );
}

cadm::Mat4 BlenderCameraStrategy::getInvProjection() {
    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return cadm::Mat4::identity();
    }
    if (camera.value()->isOrtho()) {
        return getProjection().inversedOrtho();
    }
    return getProjection().inversedPerspective();
}

void BlenderCameraStrategy::setLookTarget(const cadm::Vec3 target) {
    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return;
    }
    camera.value()->setTarget(target);
}

cadm::cadf BlenderCameraStrategy::distanceToTarget() {
    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return 1.0;
    }
    return camera.value()->getRadius();
}

bool BlenderCameraStrategy::handleCameraMove(const CameraAction action, const QPoint delta) {
    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }

    const auto pCamera = camera.value();

    switch (action) {
    case CameraAction::orbit: {
        // horizontal: rotate around world Y
        // vertical: rotate around camera's current local X
        const cadm::cadf yawAngle = -static_cast<cadm::cadf>(delta.x()) * s_sensitivity;
        const cadm::cadf pitchAngle = -static_cast<cadm::cadf>(delta.y()) * s_sensitivity;
        pCamera->m_orbitRot = (cadm::Mat3::rotY(yawAngle) * pCamera->m_orbitRot
            * cadm::Mat3::rotX(pitchAngle)).orthonormalized();
        emit
        pCamera->propertyUpdated();
        return true;
    }
    case CameraAction::pan: {
        cadm::cadf scale;
        if (pCamera->isOrtho()) {
            scale = pCamera->getOrthoHeight() / static_cast<cadm::cadf>(m_heightGetter());
        }
        else {
            // hWorld = 2 * radius * tan(fov / 2)
            // scale = hWorld / hScreen
            scale = static_cast<cadm::cadf>(
                2.0 * pCamera->getRadius() * std::tan(pCamera->getFov() / 2.0)
                / static_cast<cadm::cadf>(m_heightGetter()));
        }
        const auto translationChange = pCamera->right() * (-scale * static_cast<cadm::cadf>(delta.x()))
            + pCamera->up() * (scale * static_cast<cadm::cadf>(delta.y()));
        pCamera->setTarget(pCamera->getTarget() + translationChange);
        return true;
    }
    case CameraAction::zoomDrag: {
        const cadm::cadf factor = std::exp(
            static_cast<cadm::cadf>(-delta.y()) * static_cast<cadm::cadf>(0.01)
        );
        pCamera->setRadius(pCamera->getRadius() * factor);
        return true;
    }
    }

    return false;
}

bool BlenderCameraStrategy::handleCameraKeyAction(const CameraKeyAction action) {
    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        return false;
    }
    const auto pCamera = camera.value();
    const auto step = m_translationStep * (pCamera->isOrtho()
                                               ? pCamera->getOrthoHeight()
                                               : pCamera->getRadius());

    cadm::Vec3 newTarget;
    switch (action) {
    case CameraKeyAction::moveUp:
        newTarget = pCamera->getTarget() + pCamera->up() * step;
        break;
    case CameraKeyAction::moveDown:
        newTarget = pCamera->getTarget() - pCamera->up() * step;
        break;
    case CameraKeyAction::moveLeft:
        newTarget = pCamera->getTarget() - pCamera->right() * step;
        break;
    case CameraKeyAction::moveRight:
        newTarget = pCamera->getTarget() + pCamera->right() * step;
        break;
    default:
        return false;
    }

    pCamera->setTarget(newTarget);
    if (const auto transform = m_cameraEntity->getComponent<TransformComponent>()) {
        transform.value()->setTranslation(pCamera->getPosition());
    }
    return true;
}

bool BlenderCameraStrategy::handleWheelEvent(QWheelEvent *event) {
    const int delta = event->angleDelta().y();
    if (delta == 0) {
        return false;
    }

    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }

    if (const auto pCamera = camera.value();
        pCamera->isOrtho()) {
        auto newOrthoHeight = pCamera->getOrthoHeight();
        if (delta > 0) {
            newOrthoHeight /= pCamera->getZoomFactor();
        }
        else {
            newOrthoHeight *= pCamera->getZoomFactor();
        }
        pCamera->setOrthoHeight(newOrthoHeight);
    }
    else {
        auto newRadius = pCamera->getRadius();
        if (delta > 0) {
            newRadius /= pCamera->getZoomFactor();
        }
        else {
            newRadius *= pCamera->getZoomFactor();
        }
        pCamera->setRadius(newRadius);
    }

    return true;
}

void BlenderCameraStrategy::toggleProjection() {
    const auto camera = m_cameraEntity->getComponent<BlenderCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return;
    }
    const auto pCamera = camera.value();

    if (!pCamera->isOrtho()) {
        // switching to ortho: compute orthoHeight that matches the perspective visible height at target distance

        const auto orthoHeight = static_cast<cadm::cadf>(2.0 * pCamera->getRadius() *
            std::tan(pCamera->getFov() / 2.0));
        pCamera->setOrthoHeight(orthoHeight);
    }
    pCamera->setIsOrtho(!pCamera->isOrtho());
}
