//
// Created on 3/17/26.
//

#ifndef CAD_CAMERA_HPP
#define CAD_CAMERA_HPP

#include "cad_math/Mat3.hpp"
#include "cad_math/Mat4.hpp"
#include "cad_math/Vec3.hpp"
#include <QObject>
#include <numbers>

#include "../CameraComponent.hpp"

class BlenderCameraComponent final : public QObject, public CameraComponent {
    Q_OBJECT Q_PROPERTY(double radius READ getRadius WRITE setRadius NOTIFY radiusChanged)

    Q_PROPERTY(double fov READ getFov WRITE setFov NOTIFY fovChanged)

    Q_PROPERTY(double nearPlane READ getNearPlane WRITE setNearPlane NOTIFY nearPlaneChanged)

    Q_PROPERTY(double farPlane READ getFarPlane WRITE setFarPlane NOTIFY farPlaneChanged)

    Q_PROPERTY(double targetX READ getTargetX WRITE setTargetX NOTIFY targetXChanged)

    Q_PROPERTY(double targetY READ getTargetY WRITE setTargetY NOTIFY targetYChanged)

    Q_PROPERTY(double targetZ READ getTargetZ WRITE setTargetZ NOTIFY targetZChanged)

    Q_PROPERTY(double zoomFactor READ getZoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)

    Q_PROPERTY(bool isOrtho READ isOrtho WRITE setIsOrtho NOTIFY isOrthoChanged)

    Q_PROPERTY(double orthoHeight READ getOrthoHeight WRITE setOrthoHeight NOTIFY orthoHeightChanged)

public:
    static constexpr cadm::cadf s_minDistance = 0.01;
    static constexpr cadm::cadf s_minDistanceSq = s_minDistance * s_minDistance;

    static constexpr cadm::cadf s_radiusMin = 0.01;
    static constexpr cadm::cadf s_radiusMax = 1000.0;

    static constexpr cadm::cadf s_fovMin = 40.0 * std::numbers::pi / 180.0;
    static constexpr cadm::cadf s_fovMax = 140.0 * std::numbers::pi / 180.0;

    static constexpr cadm::cadf s_zoomFactorMin = 0.01;
    static constexpr cadm::cadf s_zoomFactorMax = 100.0;

    static constexpr cadm::cadf s_orthoHeightMin = 0.01;
    static constexpr cadm::cadf s_orthoHeightMax = 1000.0;

    explicit BlenderCameraComponent(QObject *parent = nullptr)
    : QObject(parent) {}

    [[nodiscard]] cadm::Vec3 forward() const;

    [[nodiscard]] cadm::Vec3 right() const;

    [[nodiscard]] cadm::Vec3 up() const;

    [[nodiscard]] cadm::Vec3 getPosition() const;

    [[nodiscard]] cadm::cadf getRadius() const {
        return m_radius;
    }

    [[nodiscard]] cadm::cadf getFov() const {
        return m_fov;
    }

    [[nodiscard]] cadm::cadf getNearPlane() const {
        return m_nearPlane;
    }

    [[nodiscard]] cadm::cadf getFarPlane() const {
        return m_farPlane;
    }

    [[nodiscard]] cadm::cadf getTargetX() const {
        return m_target.x;
    }

    [[nodiscard]] cadm::cadf getTargetY() const {
        return m_target.y;
    }

    [[nodiscard]] cadm::cadf getTargetZ() const {
        return m_target.z;
    }

    [[nodiscard]] cadm::Vec3 getTarget() const {
        return m_target;
    }

    [[nodiscard]] cadm::cadf getZoomFactor() const {
        return m_zoomFactor;
    }

    [[nodiscard]] bool isOrtho() const {
        return m_isOrtho;
    }

    [[nodiscard]] cadm::cadf getOrthoHeight() const {
        return m_orthoHeight;
    }

    void setTarget(const cadm::Vec3 &value);

    void setTargetX(cadm::cadf value);

    void setTargetY(cadm::cadf value);

    void setTargetZ(cadm::cadf value);

    void setRadius(cadm::cadf value);

    void setFov(cadm::cadf value);

    void setNearPlane(cadm::cadf value);

    void setFarPlane(cadm::cadf value);

    void setZoomFactor(cadm::cadf factor);

    void setIsOrtho(bool value);

    void setOrthoHeight(cadm::cadf value);

private:
    cadm::cadf m_radius{5.0};
    /// @brief Position = target + orbitRot*(0,0,radius)
    cadm::Mat3 m_orbitRot = [] {
        const cadm::Vec3 back = cadm::Vec3{1, 1, 1}.normalized();
        const cadm::Vec3 right = cadm::Vec3{0, 1, 0}.cross(back).normalized();
        return cadm::Mat3{right, back.cross(right), back};
    }();

    cadm::Vec3 m_target{};
    cadm::cadf m_nearPlane{0.1f};
    cadm::cadf m_farPlane{200.0f};
    cadm::cadf m_fov{std::numbers::pi / 4.0};

    cadm::cadf m_zoomFactor = 1.1;
    bool m_isOrtho = false;
    cadm::cadf m_orthoHeight = 5.0;

signals :
    void radiusChanged(double radius);

    void fovChanged(double fov);

    void nearPlaneChanged(double plane);

    void farPlaneChanged(double plane);

    void targetXChanged(double x);

    void targetYChanged(double y);

    void targetZChanged(double z);

    void zoomFactorChanged(double height);

    void isOrthoChanged(bool isOrtho);

    void orthoHeightChanged(double height);

    void propertyUpdated();

private:
    friend class BlenderCameraStrategy;
};

#endif //CAD_CAMERA_HPP
