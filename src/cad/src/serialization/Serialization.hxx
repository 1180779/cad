//
// Created by Radosław Głasek on 04.07.2026
//

#ifndef CAD_SERIALIZATION_HXX
#define CAD_SERIALIZATION_HXX

#include <QJsonDocument>
#include <valijson/validation_results.hpp>

#include "EntitySpec.hpp"
#include "components/AllComponents.hxx"

class Scene;

namespace serialization {
    /// @brief Validate a scene json document @ref json against @p jsonSchema
    /// json schema
    std::optional<valijson::ValidationResults> validateJson(QJsonDocument jsonSchema, QJsonDocument json);

    /// @brief Serialize @p scene into JSON document
    QJsonDocument toJson(Scene &scene);

    /// @brief Rebuild entities from a scene JSON document into scene. Points
    /// are created first and control-point references are resolved against them
    /// @note Entities of an unrecognized/unimplemented objectType are skipped
    std::vector<Entity*> fromJson(Scene &scene, const QJsonDocument &json);
}

#endif //CAD_SERIALIZATION_HXX
