//
// Created on 3/15/26.
//

#ifndef CAD_TRANSFORM_H
#define CAD_TRANSFORM_H

#include <QObject>

#include "Entity.hpp"
#include <cad_math/Vec3.hpp>
#include <cad_math/Mat4.hpp>

class TransformComponent final : public QObject, public Component {
    Q_OBJECT

public:
    TransformComponent();

    explicit TransformComponent(const cadm::Vec3 &translation);

    TransformComponent(const cadm::Vec3 &translation, const cadm::Vec3 &rotation);

    [[nodiscard]] const cadm::Mat4& getModelMatrix() const;

    [[nodiscard]] cadm::Vec3 getTranslation() const {
        return m_translation;
    }

    [[nodiscard]] cadm::Vec3 getScale() const {
        return m_scale;
    }

    [[nodiscard]] cadm::Vec3 getRotation() const {
        return m_rotation;
    }

    void setTranslation(const cadm::Vec3 &translation);

    void setScale(const cadm::Vec3 &scale);

    void setRotation(const cadm::Vec3 &rotation);

    signals  :

    

    void translationXChanged(double value);

    void translationYChanged(double value);

    void translationZChanged(double value);

    void scaleXChanged(double value);

    void scaleYChanged(double value);

    void scaleZChanged(double value);

    void rotationXChanged(double value);

    void rotationYChanged(double value);

    void rotationZChanged(double value);

private:
    cadm::Vec3 m_translation{};

    // euler angles for now
    // TODO: replace with quaternions

    cadm::Vec3 m_rotation{}; // Z, Y, X
    cadm::Vec3 m_scale{1.0f, 1.0f, 1.0f};

    mutable cadm::Mat4 m_modelMatrix = cadm::Mat4::identity();
    mutable bool m_isDirty = true;
};

#endif //CAD_TRANSFORM_H
