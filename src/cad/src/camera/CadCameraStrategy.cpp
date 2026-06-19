//
// Created on 3/23/26.
//

#include "CadCameraStrategy.hpp"

#include "../CheckMacros.hpp"
#include "../components/CadCameraComponent.hpp"
#include "../components/BlenderCameraComponent.hpp"
#include "../components/TransformComponent.hpp"

CadCameraStrategy::CadCameraStrategy(
    Entity *cameraEntity,
    const std::function<int()> &widthGetter,
    const std::function<int()> &heightGetter
) : ICameraStrategy(cameraEntity, widthGetter, heightGetter) {}

cadm::mat4 CadCameraStrategy::getView() {
    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();
    const auto view = cadm::mat4::lookAtRH(pCamera->getPosition(), pCamera->getTarget(), pCamera->up());
    return view;
}

cadm::mat4 CadCameraStrategy::getProjection() {
    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return cadm::mat4::identity();
    }
    const auto pCamera = camera.value();
    const auto height = pCamera->getOrthoHeight();
    const auto width = height * pCamera->getAspectRatio();

    const auto projection = cadm::mat4::ortho(
        static_cast<cadm::cadf>(-width / 2.0),
        static_cast<cadm::cadf>(width / 2.0),
        -static_cast<cadm::cadf>(height / 2.0),
        static_cast<cadm::cadf>(height / 2.0),
        pCamera->getNearPlane(),
        pCamera->getFarPlane()
    );
    return projection;
}

void CadCameraStrategy::setLookTarget(const cadm::vec3 target) {
    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return;
    }
    const auto pCamera = camera.value();
    const auto offset = pCamera->getPosition() - pCamera->getTarget();
    const auto newPosition = target + offset;
    pCamera->setPosition(newPosition);
    pCamera->setTarget(target);

    if (const auto transform = m_cameraEntity->getComponent<TransformComponent>()) {
        transform.value()->setTranslation(newPosition);
    }
}

void CadCameraStrategy::handleOrbit(const QPoint mouseDelta, CadCameraComponent *const pCamera) const {
    // TODO(raycast-pivot): rotation pivot should be the ray-scene intersection point under the cursor.
    //  Currently rotates around target
    const auto polarAngleChange = -static_cast<cadm::cadf>(mouseDelta.y()) * pCamera->getRotationSpeed();
    const auto azimuthAngleChange = static_cast<cadm::cadf>(mouseDelta.x()) * pCamera->getRotationSpeed();

    const auto pivot = pCamera->getTarget();
    const auto relPosition = pCamera->getPosition() - pivot;

    const auto polarRot = cadm::mat4::rotAxis(polarAngleChange, pCamera->right()).upperLeft3x3();
    auto newRelPos = polarRot * relPosition;
    auto newUp = polarRot * pCamera->up();

    const auto azimuthRot = cadm::mat4::rotAxis(azimuthAngleChange, pCamera->getWorldUp()).upperLeft3x3();
    newRelPos = azimuthRot * newRelPos;
    newUp = azimuthRot * newUp;

    newRelPos = newRelPos.safeNormalized(cadm::vec3::unitZ()) * relPosition.length();
    const auto newPosition = pivot + newRelPos;

    // Re-orthogonalize up
    const auto newForward = (-newRelPos).safeNormalized(cadm::vec3::unitZ());
    const auto finalUp = (newUp - newForward * newUp.dot(newForward)).safeNormalized(pCamera->getWorldUp());

    pCamera->setPosition(newPosition);
    pCamera->setUp(finalUp);

    if (const auto transform = m_cameraEntity->getComponent<TransformComponent>();
        transform.has_value()) {
        transform.value()->setTranslation(newPosition);
    }
}

void CadCameraStrategy::handlePan(const QPoint mouseDelta, CadCameraComponent *const pCamera) const {
    const cadm::cadf changeX = -pCamera->getAspectRatio() * pCamera->getOrthoHeight() * static_cast<cadm::cadf>(
        mouseDelta.x()) / static_cast<cadm::cadf>(m_widthGetter());
    const cadm::cadf changeY = pCamera->getOrthoHeight() * static_cast<cadm::cadf>(mouseDelta.y()) / static_cast<
        cadm::cadf>(m_heightGetter());
    const auto translationChange = pCamera->right() * changeX + pCamera->up() * changeY;
    pCamera->setPosition(pCamera->getPosition() + translationChange);
    pCamera->setTarget(pCamera->getTarget() + translationChange);
}

bool CadCameraStrategy::handleCameraMove(const CameraAction action, const QPoint mouseDelta) {
    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera.has_value()) {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }

    const auto pCamera = camera.value();

    switch (action) {
    case CameraAction::Orbit:
        handleOrbit(mouseDelta, pCamera);
        return true;
    case CameraAction::Pan:
        handlePan(mouseDelta, pCamera);
        return true;
    case CameraAction::ZoomDrag:
        const cadm::cadf factor = std::exp(
            static_cast<cadm::cadf>(-mouseDelta.y()) * static_cast<cadm::cadf>(0.01)
        );
        pCamera->setOrthoHeight(pCamera->getOrthoHeight() * factor);
        return true;
    }

    return false;
}

bool CadCameraStrategy::handleCameraKeyAction(const CameraKeyAction action) {
    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera) {
        return false;
    }
    const auto pCamera = camera.value();
    const auto step = m_translationStep * pCamera->getOrthoHeight();

    cadm::vec3 offset;
    switch (action) {
    case CameraKeyAction::MoveUp:
        offset = pCamera->up() * step;
        break;
    case CameraKeyAction::MoveDown:
        offset = -pCamera->up() * step;
        break;
    case CameraKeyAction::MoveLeft:
        offset = -pCamera->right() * step;
        break;
    case CameraKeyAction::MoveRight:
        offset = pCamera->right() * step;
        break;
    default:
        return false;
    }

    pCamera->setPosition(pCamera->getPosition() + offset);
    pCamera->setTarget(pCamera->getTarget() + offset);
    if (const auto transform = m_cameraEntity->getComponent<TransformComponent>()) {
        transform.value()->setTranslation(pCamera->getPosition());
    }
    return true;
}

bool CadCameraStrategy::handleWheelEvent(QWheelEvent *event) {
    const int delta = event->angleDelta().y();
    if (delta == 0) {
        return false;
    }

    const auto camera = m_cameraEntity->getComponent<CadCameraComponent>();
    if (!camera) {
        EXPECTED_COMPONENT_MISSING();
        return false;
    }
    const auto pCamera = camera.value();

    const auto pos = event->position();
    const auto nx = pos.x() / m_widthGetter() * 2.0 - 1.0;
    const auto ny = 1.0 - pos.y() / m_heightGetter() * 2.0;

    const auto oldHeight = pCamera->getOrthoHeight();
    const auto oldWidth = oldHeight * pCamera->getAspectRatio();

    if (delta > 0) {
        const auto newOrthoHeight = pCamera->getOrthoHeight() / pCamera->getZoomFactor();
        pCamera->setOrthoHeight(newOrthoHeight);
    }
    else {
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

    if (const auto transform = m_cameraEntity->getComponent<TransformComponent>()) {
        transform.value()->setTranslation(position);
    }
    return true;
}
