//
// Created on 3/23/26.
//

#ifndef CAD_CADCAMERACOMPOONENT_HPP
#define CAD_CADCAMERACOMPOONENT_HPP

#include "CameraComponent.hpp"
#include "cad_math/common.hpp"
#include "cad_math/vec3.hpp"
#include <QObject>

class CadCameraComponent final : public QObject, public CameraComponent {
    Q_OBJECT Q_PROPERTY(double positionX READ getPositionX WRITE setPositionX NOTIFY positionXChanged)

    Q_PROPERTY(double positionY READ getPositionY WRITE setPositionY NOTIFY positionYChanged)

    Q_PROPERTY(double positionZ READ getPositionZ WRITE setPositionZ NOTIFY positionZChanged)

    Q_PROPERTY(double targetX READ getTargetX WRITE setTargetX NOTIFY targetXChanged)

    Q_PROPERTY(double targetY READ getTargetY WRITE setTargetY NOTIFY targetYChanged)

    Q_PROPERTY(double targetZ READ getTargetZ WRITE setTargetZ NOTIFY targetZChanged)

    Q_PROPERTY(double worldUpX READ getWorldUpX WRITE setWorldUpX NOTIFY worldUpXChanged)

    Q_PROPERTY(double worldUpY READ getWorldUpY WRITE setWorldUpY NOTIFY worldUpYChanged)

    Q_PROPERTY(double worldUpZ READ getWorldUpZ WRITE setWorldUpZ NOTIFY worldUpZChanged)

    Q_PROPERTY(double upX READ getUpX WRITE setUpX NOTIFY upXChanged)

    Q_PROPERTY(double upY READ getUpY WRITE setUpY NOTIFY upYChanged)

    Q_PROPERTY(double upZ READ getUpZ WRITE setUpZ NOTIFY upZChanged)

    Q_PROPERTY(double nearPlane READ getNearPlane WRITE setNearPlane NOTIFY nearPlaneChanged)

    Q_PROPERTY(double farPlane READ getFarPlane WRITE setFarPlane NOTIFY farPlaneChanged)

    Q_PROPERTY(double orthoHeight READ getOrthoHeight WRITE setOrthoHeight NOTIFY orthoHeightChanged)

    Q_PROPERTY(double rotationSpeed READ getRotationSpeed WRITE setRotationSpeed NOTIFY rotationSpeedChanged)

    Q_PROPERTY(double zoomFactor READ getZoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)

public:
    static constexpr cadm::cadf s_nearPlaneMin = -CameraComponent::s_farPlaneMax;
    static constexpr cadm::cadf s_nearPlaneMax = 0;
    static constexpr cadm::cadf s_farPlaneMin = 0;
    static constexpr cadm::cadf s_farPlaneMax = CameraComponent::s_farPlaneMax;

    static constexpr cadm::cadf s_minDistance = 0.01;
    static constexpr cadm::cadf s_minDistanceSq = s_minDistance * s_minDistance;

    static constexpr double s_positionMin = std::numeric_limits<double>::lowest();
    static constexpr double s_positionMax = std::numeric_limits<double>::max();

    static constexpr cadm::cadf s_orthoHeightMin = 0.01;
    static constexpr cadm::cadf s_orthoHeightMax = 100.0;

    static constexpr cadm::cadf s_rotationSpeedMin = 0.01;
    static constexpr cadm::cadf s_rotationSpeedMax = 100.0;

    static constexpr cadm::cadf s_zoomFactorMin = 0.01;
    static constexpr cadm::cadf s_zoomFactorMax = 100.0;

    [[nodiscard]] cadm::vec3 forward() const;

    [[nodiscard]] cadm::vec3 right() const;

    [[nodiscard]] cadm::vec3 up() const;

    [[nodiscard]] cadm::vec3 getPosition() const {
        return m_position;
    }

    [[nodiscard]] cadm::cadf getPositionX() const {
        return m_position.x;
    }

    [[nodiscard]] cadm::cadf getPositionY() const {
        return m_position.y;
    }

    [[nodiscard]] cadm::cadf getPositionZ() const {
        return m_position.z;
    }

    [[nodiscard]] cadm::vec3 getTarget() const {
        return m_target;
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

    [[nodiscard]] cadm::vec3 getWorldUp() const {
        return m_worldUp;
    }

    [[nodiscard]] cadm::cadf getWorldUpX() const {
        return m_worldUp.x;
    }

    [[nodiscard]] cadm::cadf getWorldUpY() const {
        return m_worldUp.y;
    }

    [[nodiscard]] cadm::cadf getWorldUpZ() const {
        return m_worldUp.z;
    }

    [[nodiscard]] cadm::vec3 getUp() const {
        return m_up;
    }

    [[nodiscard]] cadm::cadf getUpX() const {
        return m_up.x;
    }

    [[nodiscard]] cadm::cadf getUpY() const {
        return m_up.y;
    }

    [[nodiscard]] cadm::cadf getUpZ() const {
        return m_up.z;
    }

    [[nodiscard]] cadm::cadf getNearPlane() const {
        return m_nearPlane;
    }

    [[nodiscard]] cadm::cadf getFarPlane() const {
        return m_farPlane;
    }

    [[nodiscard]] cadm::cadf getOrthoHeight() const {
        return m_orthoHeight;
    }

    [[nodiscard]] cadm::cadf getRotationSpeed() const {
        return m_rotationSpeed;
    }

    [[nodiscard]] cadm::cadf getZoomFactor() const {
        return m_zoomFactor;
    }

    void setPosition(const cadm::vec3 &position);

    void setPositionX(cadm::cadf x);

    void setPositionY(cadm::cadf y);

    void setPositionZ(cadm::cadf z);

    void setTarget(const cadm::vec3 &target);

    void setTargetX(cadm::cadf x);

    void setTargetY(cadm::cadf y);

    void setTargetZ(cadm::cadf z);

    void setWorldUp(const cadm::vec3 &worldUp);

    void setWorldUpX(cadm::cadf x);

    void setWorldUpY(cadm::cadf y);

    void setWorldUpZ(cadm::cadf z);

    void setUp(const cadm::vec3 &up);

    void setUpX(cadm::cadf x);

    void setUpY(cadm::cadf y);

    void setUpZ(cadm::cadf z);

    void setNearPlane(cadm::cadf nearPlane);

    void setFarPlane(cadm::cadf farPlane);

    void setOrthoHeight(cadm::cadf height);

    void setZoomFactor(cadm::cadf factor);

    void setRotationSpeed(cadm::cadf rotationSpeed);

private:
    cadm::vec3 m_position{};
    cadm::vec3 m_target{};
    cadm::vec3 m_worldUp = cadm::vec3::unitY();
    cadm::vec3 m_up = cadm::vec3::unitY();

    cadm::cadf m_nearPlane{-100.0};
    cadm::cadf m_farPlane{100.0};

    cadm::cadf m_orthoHeight{2.0};

    cadm::cadf m_rotationSpeed{0.005};
    cadm::cadf m_zoomFactor{1.1};

    signals  :

    

    void positionChanged(double position);

    void positionXChanged(double x);

    void positionYChanged(double y);

    void positionZChanged(double z);

    void targetChanged(double target);

    void targetXChanged(double x);

    void targetYChanged(double y);

    void targetZChanged(double z);

    void worldUpChanged(double worldUp);

    void worldUpXChanged(double x);

    void worldUpYChanged(double y);

    void worldUpZChanged(double z);

    void upXChanged(double x);

    void upYChanged(double y);

    void upZChanged(double z);

    void nearPlaneChanged(double nearPlane);

    void farPlaneChanged(double farPlane);

    void orthoHeightChanged(double height);

    void rotationSpeedChanged(double height);

    void zoomFactorChanged(double height);

    void propertyUpdated();
};

#endif //CAD_CADCAMERACOMPOONENT_HPP
