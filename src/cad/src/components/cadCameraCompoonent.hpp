//
// Created on 3/23/26.
//

#ifndef CAD_CADCAMERACOMPOONENT_HPP
#define CAD_CADCAMERACOMPOONENT_HPP

#include "ICamera.hpp"
#include "cad_math/common.h"
#include "cad_math/vec3.h"
#include <QObject>

class cadCameraComponent final : public QObject, public CameraComponent
{
    Q_OBJECT

    Q_PROPERTY(double positionX READ getPositionX WRITE setPositionX NOTIFY positionXChanged)
    Q_PROPERTY(double positionY READ getPositionY WRITE setPositionY NOTIFY positionYChanged)
    Q_PROPERTY(double positionZ READ getPositionZ WRITE setPositionZ NOTIFY positionZChanged)

    Q_PROPERTY(double targetX READ getTargetX WRITE setTargetX NOTIFY targetXChanged)
    Q_PROPERTY(double targetY READ getTargetY WRITE setTargetY NOTIFY targetYChanged)
    Q_PROPERTY(double targetZ READ getTargetZ WRITE setTargetZ NOTIFY targetZChanged)

    Q_PROPERTY(double worldUpX READ getWorldUpX WRITE setWorldUpX NOTIFY worldUpXChanged)
    Q_PROPERTY(double worldUpY READ getWorldUpY WRITE setWorldUpY NOTIFY worldUpYChanged)
    Q_PROPERTY(double worldUpZ READ getWorldUpZ WRITE setWorldUpZ NOTIFY worldUpZChanged)

    Q_PROPERTY(double nearPlane READ getNearPlane WRITE setNearPlane NOTIFY nearPlaneChanged)
    Q_PROPERTY(double farPlane READ getFarPlane WRITE setFarPlane NOTIFY farPlaneChanged)

    Q_PROPERTY(double orthoHeight READ getOrthoHeight WRITE setOrthoHeight NOTIFY orthoHeightChanged)

public:
    static constexpr cadm::cadf s_minDistance = 0.01;
    static constexpr cadm::cadf s_minDistanceSq = s_minDistance * s_minDistance;

    static constexpr double s_positionMin = std::numeric_limits<double>::lowest();
    static constexpr double s_positionMax = std::numeric_limits<double>::max();

    static constexpr cadm::cadf s_orthoHeightMin = 0.01;
    static constexpr cadm::cadf s_orthoHeightMax = 100.0;

    [[nodiscard]] cadm::vec3 forward() const;
    [[nodiscard]] cadm::vec3 right() const;
    [[nodiscard]] cadm::vec3 up() const;

    [[nodiscard]] cadm::vec3 getPosition() const { return m_position; }
    [[nodiscard]] cadm::cadf getPositionX() const { return m_position.x; }
    [[nodiscard]] cadm::cadf getPositionY() const { return m_position.y; }
    [[nodiscard]] cadm::cadf getPositionZ() const { return m_position.z; }

    [[nodiscard]] cadm::vec3 getTarget() const { return m_target; }
    [[nodiscard]] cadm::cadf getTargetX() const { return m_target.x; }
    [[nodiscard]] cadm::cadf getTargetY() const { return m_target.y; }
    [[nodiscard]] cadm::cadf getTargetZ() const { return m_target.z; }

    [[nodiscard]] cadm::vec3 getWorldUp() const { return m_worldUp; }
    [[nodiscard]] cadm::cadf getWorldUpX() const { return m_worldUp.x; }
    [[nodiscard]] cadm::cadf getWorldUpY() const { return m_worldUp.y; }
    [[nodiscard]] cadm::cadf getWorldUpZ() const { return m_worldUp.z; }

    [[nodiscard]] cadm::cadf getNearPlane() const { return m_nearPlane; }
    [[nodiscard]] cadm::cadf getFarPlane() const { return m_farPlane; }
    [[nodiscard]] cadm::cadf getOrthoHeight() const { return m_orthoHeight; }

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

    void setNearPlane(cadm::cadf nearPlane);
    void setFarPlane(cadm::cadf farPlane);
    void setOrthoHeight(cadm::cadf height);

private:
    cadm::vec3 m_position{};
    cadm::vec3 m_target{};
    cadm::vec3 m_worldUp = cadm::vec3::unitY();

    cadm::cadf m_nearPlane{0.1f};
    cadm::cadf m_farPlane{100.0f};

    cadm::cadf m_orthoHeight{2.0f};

signals:
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

    void nearPlaneChanged(double nearPlane);
    void farPlaneChanged(double farPlane);
    void orthoHeightChanged(double height);

    void propertyUpdated();
};


#endif //CAD_CADCAMERACOMPOONENT_HPP
