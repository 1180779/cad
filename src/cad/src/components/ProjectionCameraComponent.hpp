//
// Created on 3/17/26.
//

#ifndef CAD_CAMERA_HPP
#define CAD_CAMERA_HPP

#include "cad_math/mat4.hpp"
#include "cad_math/vec3.hpp"
#include <QObject>
#include <numbers>

#include "CameraComponent.hpp"

class ProjectionCameraComponent final : public QObject, public CameraComponent
{
    Q_OBJECT

    Q_PROPERTY(double radius READ getRadius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(double azimuthAngle READ getAzimuthAngle WRITE setAzimuthAngle NOTIFY azimuthAngleChanged)
    Q_PROPERTY(double polarAngle READ getPolarAngle WRITE setPolarAngle NOTIFY polarAngleChanged)

    Q_PROPERTY(double fov READ getFov WRITE setFov NOTIFY fovChanged)

    Q_PROPERTY(double nearPlane READ getNearPlane WRITE setNearPlane NOTIFY nearPlaneChanged)
    Q_PROPERTY(double farPlane READ getFarPlane WRITE setFarPlane NOTIFY farPlaneChanged)

    Q_PROPERTY(double targetX READ getTargetX WRITE setTargetX NOTIFY targetXChanged)
    Q_PROPERTY(double targetY READ getTargetY WRITE setTargetY NOTIFY targetYChanged)
    Q_PROPERTY(double targetZ READ getTargetZ WRITE setTargetZ NOTIFY targetZChanged)

    Q_PROPERTY(double zoomFactor READ getZoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)

public:
    static constexpr cadm::cadf s_minDistance = 0.01;
    static constexpr cadm::cadf s_minDistanceSq = s_minDistance * s_minDistance;

    static constexpr cadm::cadf s_radiusMin = 0.01;
    static constexpr cadm::cadf s_radiusMax = 1000.0;

    static constexpr cadm::cadf s_azimuthAngleMin = 0.0;
    static constexpr cadm::cadf s_azimuthAngleMax = 2.0 * std::numbers::pi;

    static constexpr cadm::cadf s_polarAngleMin = -80.0 * std::numbers::pi / 180.0;
    static constexpr cadm::cadf s_polarAngleMax = 80.0 * std::numbers::pi / 180.0;

    static constexpr cadm::cadf s_fovMin = 40.0 * std::numbers::pi / 180.0;
    static constexpr cadm::cadf s_fovMax = 140.0 * std::numbers::pi / 180.0;

    static constexpr cadm::cadf s_zoomFactorMin = 0.01;
    static constexpr cadm::cadf s_zoomFactorMax = 100.0;

    explicit ProjectionCameraComponent(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    [[nodiscard]] cadm::vec3 forward() const;
    [[nodiscard]] cadm::vec3 right() const;
    [[nodiscard]] cadm::vec3 up() const;
    [[nodiscard]] cadm::vec3 getPosition() const;

    [[nodiscard]] cadm::cadf getRadius() const { return m_radius; }
    [[nodiscard]] cadm::cadf getAzimuthAngle() const { return m_azimuthAngle; }
    [[nodiscard]] cadm::cadf getPolarAngle() const { return m_polarAngle; }
    [[nodiscard]] cadm::cadf getFov() const { return m_fov; }
    [[nodiscard]] cadm::cadf getNearPlane() const { return m_nearPlane; }
    [[nodiscard]] cadm::cadf getFarPlane() const { return m_farPlane; }

    [[nodiscard]] cadm::cadf getTargetX() const { return m_target.x; }
    [[nodiscard]] cadm::cadf getTargetY() const { return m_target.y; }
    [[nodiscard]] cadm::cadf getTargetZ() const { return m_target.z; }
    [[nodiscard]] cadm::vec3 getTarget() const { return m_target; }
    [[nodiscard]] cadm::vec3 getWorldUp() const { return m_worldUp; }

    [[nodiscard]] cadm::cadf getZoomFactor() const { return m_zoomFactor; }

    void setTarget(const cadm::vec3 &value);
    void setTargetX(cadm::cadf value);
    void setTargetY(cadm::cadf value);
    void setTargetZ(cadm::cadf value);

    void setRadius(cadm::cadf value);
    void setAzimuthAngle(cadm::cadf value);
    void setPolarAngle(cadm::cadf value);
    void setFov(cadm::cadf value);
    void setNearPlane(cadm::cadf value);
    void setFarPlane(cadm::cadf value);

    void setZoomFactor(cadm::cadf factor);

private:
    cadm::cadf m_radius{5.0};
    cadm::cadf m_azimuthAngle{};
    cadm::cadf m_polarAngle{};

    cadm::vec3 m_target{};
    cadm::vec3 m_worldUp = cadm::vec3::unitY();
    cadm::cadf m_nearPlane{0.1f};
    cadm::cadf m_farPlane{100.0f};
    cadm::cadf m_fov{std::numbers::pi / 4.0};

    cadm::cadf m_zoomFactor = 1.1;

signals :
    void radiusChanged(double radius);
    void azimuthAngleChanged(double angle);
    void polarAngleChanged(double angle);

    void fovChanged(double fov);

    void nearPlaneChanged(double plane);
    void farPlaneChanged(double plane);

    void targetXChanged(double x);
    void targetYChanged(double y);
    void targetZChanged(double z);

    void zoomFactorChanged(double height);

    void propertyUpdated();

private:
    friend class ProjectionCameraStrategy;
};


#endif //CAD_CAMERA_HPP
