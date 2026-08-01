//
// Created by Radosław Głasek on 04.07.2026
//

// ReSharper disable CppInconsistentNaming
#include "Serialization.hxx"

#include <algorithm>
#include <unordered_map>

#include <valijson/adapters/qtjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

#include <cad_math/Quaternion.hxx>

#include "Scene.hpp"

using valijson::Schema;
using valijson::SchemaParser;
using valijson::Subschema;
using valijson::Validator;
using valijson::ValidationResults;
using namespace valijson::adapters;
using namespace valijson::constraints;

namespace { namespace schemaType {
        constexpr auto torus = "torus";
        // constexpr auto chain = "chain";
        constexpr auto bezierC0 = "bezierC0";
        constexpr auto bezierC2 = "bezierC2";
        constexpr auto interpolatedC2 = "interpolatedC2";
        constexpr auto bezierSurfaceC0 = "bezierSurfaceC0";
        constexpr auto bezierSurfaceC2 = "bezierSurfaceC2";
    }

    namespace schemaProperties {
        constexpr auto points = "points";
        constexpr auto geometry = "geometry";
        constexpr auto name = "name";
        constexpr auto objectType = "objectType";
        constexpr auto id = "id";
        constexpr auto controlPoints = "controlPoints";
        constexpr auto size = "size";
        constexpr auto samples = "samples";
        constexpr auto position = "position";
        constexpr auto rotation = "rotation";
        constexpr auto scale = "scale";
        constexpr auto smallRadius = "smallRadius";
        constexpr auto largeRadius = "largeRadius";
        constexpr auto x = "x";
        constexpr auto y = "y";
        constexpr auto z = "z";
        constexpr auto w = "w";
        constexpr auto u = "u";
        constexpr auto v = "v";
    }

    namespace st = schemaType;
    namespace sp = schemaProperties;

    /// @brief entity id to PointHandle map type
    using PointIdToHandleMap = std::unordered_map<int, PointHandle>;

    /// @brief PointHandle to entity id map type
    using PointHandleToIdMap = std::unordered_map<PointHandle, int>;

    cadm::Vec3 vecFromJson(const QJsonObject &json) {
        return {
            static_cast<cadm::cadf>(json[sp::x].toDouble()),
            static_cast<cadm::cadf>(json[sp::y].toDouble()),
            static_cast<cadm::cadf>(json[sp::z].toDouble())
        };
    }

    QJsonObject vecToJson(const cadm::Vec3 &v) {
        return {{sp::x, v.x}, {sp::y, v.y}, {sp::z, v.z}};
    }

    cadm::Quat quatFromJson(const QJsonObject &json) {
        return cadm::Quat(
            static_cast<cadm::cadf>(json[sp::w].toDouble()),
            static_cast<cadm::cadf>(json[sp::x].toDouble()),
            static_cast<cadm::cadf>(json[sp::y].toDouble()),
            static_cast<cadm::cadf>(json[sp::z].toDouble())
        );
    }

    QJsonObject quatToJson(const cadm::Quat &q) {
        return {{sp::w, q[0]}, {sp::x, q[1]}, {sp::y, q[2]}, {sp::z, q[3]}};
    }

    QJsonObject uint2ToJson(int uVal, int vVal) {
        return {{sp::u, uVal}, {sp::v, vVal}};
    }

    std::string nameOr(const QJsonObject &json, const std::string &fallback) {
        const auto n = json[sp::name].toString().toStdString();
        return n.empty()
                   ? fallback
                   : n;
    }

    std::vector<PointHandle> controlPointsFromJson(const QJsonArray &json, const PointIdToHandleMap &map) {
        std::vector<PointHandle> out;
        out.reserve(json.size());
        for (const auto &ref : json) {
            out.push_back(map.at(ref.toObject()[sp::id].toInt()));
        }
        return out;
    }

    QJsonArray controlPointsToJson(const std::vector<PointHandle> &cps, const PointHandleToIdMap &map) {
        QJsonArray arr;
        for (const auto h : cps) {
            arr.append(QJsonObject{{sp::id, static_cast<int>(map.at(h))}});
        }
        return arr;
    }

    /// @brief Grid of a patch as the format expects it: a surface closed along
    /// u is written flat, with its @p seam first rows repeated at the end
    std::vector<PointHandle> patchGridForJson(const PatchComponent *patch, const int seam) {
        const auto &cps = patch->getControlPoints();
        if (patch->getWrap() != WrapDirection::u) {
            return cps;
        }
        std::vector<PointHandle> out = cps;
        const auto cols = static_cast<std::ptrdiff_t>(patch->getCols());
        out.insert(out.end(), cps.begin(), cps.begin() + static_cast<std::ptrdiff_t>(seam) * cols);
        return out;
    }

    /// @brief Row count matching <tt>patchGridForJson</tt>
    int patchJsonRows(const PatchComponent *patch, const int seam) {
        return patch->getWrap() == WrapDirection::u
                   ? patch->getRows() + seam
                   : patch->getRows();
    }

    /// @brief Reuses the JSON id as both the point's EntityId and its handle
    /// (via @ref Scene::createEntityWithId / @ref Scene::attachPointComponent),
    /// so a save
    /// @pre ids are unique across the whole file (points and geometry combined)
    Entity* pointFromJson(Scene &scene, const QJsonObject &json, PointIdToHandleMap &map) {
        const auto id = json[sp::id].toInt();
        const auto handle = static_cast<PointHandle>(id);
        Entity *e = scene.createEntityWithId(static_cast<EntityId>(id), nameOr(json, "Point"));
        scene.attachPointComponent(e, handle, vecFromJson(json[sp::position].toObject()));
        map.emplace(id, handle);
        return e;
    }

    Entity* torusFromJson(Scene &scene, const QJsonObject &json) {
        const auto samples = json[sp::samples].toObject();
        Entity *e = scene.createEntityWithId(json[sp::id].toInt(), nameOr(json, "Torus"));
        const auto t = e->addComponent<TransformComponent>();
        t->setTranslation(vecFromJson(json[sp::position].toObject()));
        t->setRotation(quatFromJson(json[sp::rotation].toObject()).toEuler());
        t->setScale(vecFromJson(json[sp::scale].toObject()));
        const auto torus = e->addComponent<TorusComponent>();
        torus->setMajorRadius(static_cast<cadm::cadf>(json[sp::largeRadius].toDouble()));
        torus->setMinorRadius(static_cast<cadm::cadf>(json[sp::smallRadius].toDouble()));
        torus->setMajorSegments(samples[sp::u].toInt());
        torus->setMinorSegments(samples[sp::v].toInt());
        return e;
    }

    template <typename C>
    concept isCurveComponent = requires(C c, PointHandle h) {
        std::is_base_of_v<Component, C>;
        { c.addControlPoint(h) };
    };

    template <typename T> requires isCurveComponent<T>
    Entity* controlPointCurveFromJson(
        Scene &scene,
        const QJsonObject &json,
        const PointIdToHandleMap &map,
        const std::string &defaultName
    ) {
        Entity *e = scene.createEntityWithId(json[sp::id].toInt(), nameOr(json, defaultName));
        auto *curve = e->addComponent<T>(&scene.getPointRegistry());
        for (const auto h : controlPointsFromJson(json[sp::controlPoints].toArray(), map)) {
            curve->addControlPoint(h);
        }
        return e;
    }

    Entity* bezierC0FromJson(Scene &scene, const QJsonObject &json, const PointIdToHandleMap &map) {
        return controlPointCurveFromJson<BezierC0Component>(scene, json, map, "BezierC0");
    }

    Entity* bezierC2FromJson(Scene &scene, const QJsonObject &json, const PointIdToHandleMap &map) {
        return controlPointCurveFromJson<BezierC2Component>(scene, json, map, "BezierC2");
    }

    Entity* interpolatedC2FromJson(Scene &scene, const QJsonObject &json, const PointIdToHandleMap &map) {
        return controlPointCurveFromJson<InterpC2Component>(scene, json, map, "InterpC2");
    }

    /// @brief Rebuild a joined-patch surface. The JSON grid is row-major (rows
    /// = size.v, cols = size.u) and stores the control points explicitly,
    /// including any duplicated wrap-seam points
    /// @brief Whether the grid's last @p seam rows repeat its first @p seam
    /// rows
    bool hasDuplicatedSeamRows(
        const std::vector<PointHandle> &cps,
        const int rows,
        const int cols,
        const int seam
    ) {
        if (rows <= seam) {
            return false;
        }
        const auto first = cps.begin();
        const auto last = cps.begin() + static_cast<std::ptrdiff_t>(rows - seam) * cols;
        return std::equal(last, last + static_cast<std::ptrdiff_t>(seam) * cols, first);
    }

    Entity* patchFromJson(Scene &scene, const QJsonObject &json, const PointIdToHandleMap &map, const bool c2) {
        auto cps = controlPointsFromJson(json[sp::controlPoints].toArray(), map);
        const auto size = json[sp::size].toObject();
        const int cols = size[sp::u].toInt();
        int rows = size[sp::v].toInt();
        const int patchCountX = c2
                                    ? cols - 3
                                    : (cols - 1) / 3;
        const int patchCountY = c2
                                    ? rows - 3
                                    : (rows - 1) / 3;

        // a cylinder is stored flat, with its seam rows repeated at the end;
        // drop them and mark the surface as closed instead
        const int seam = c2
                             ? 3
                             : 1;
        const bool wrapRows = hasDuplicatedSeamRows(cps, rows, cols, seam);
        if (wrapRows) {
            rows -= seam;
            cps.resize(static_cast<size_t>(rows) * cols);
        }

        Entity *e = scene.createEntityWithId(
            json[sp::id].toInt(),
            nameOr(
                json,
                c2
                    ? "Patch C2"
                    : "Patch C0"
            )
        );
        PatchComponent *patch = c2
                                    ? static_cast<PatchComponent*>(
                                        e->addComponent<PatchC2Component>(&scene.getPointRegistry())
                                    )
                                    : e->addComponent<PatchC0Component>(&scene.getPointRegistry());
        patch->setGrid(
            std::move(cps),
            rows,
            cols,
            wrapRows
                ? WrapDirection::u
                : WrapDirection::none,
            patchCountX,
            patchCountY
        );

        const auto samples = json[sp::samples].toObject();
        patch->setGridDivisionsU(samples[sp::u].toInt());
        patch->setGridDivisionsV(samples[sp::v].toInt());
        return e;
    }

    Entity* entityFromJson(Scene &scene, const QJsonObject &json, const PointIdToHandleMap &map) {
        const auto type = json[sp::objectType].toString().toStdString();
        if (type == st::torus) {
            return torusFromJson(scene, json);
        }
        if (type == st::bezierC0) {
            return bezierC0FromJson(scene, json, map);
        }
        if (type == st::bezierC2) {
            return bezierC2FromJson(scene, json, map);
        }
        if (type == st::interpolatedC2) {
            return interpolatedC2FromJson(scene, json, map);
        }
        if (type == st::bezierSurfaceC0) {
            return patchFromJson(scene, json, map, false);
        }
        if (type == st::bezierSurfaceC2) {
            return patchFromJson(scene, json, map, true);
        }
        // skip "chain"
        return nullptr;
    }
}

std::optional<ValidationResults> serialization::validateJson(
    // ReSharper disable once CppPassValueParameterByConstReference
    QJsonDocument jsonSchema // NOLINT(*-unnecessary-value-param)
    ,
    // ReSharper disable once CppPassValueParameterByConstReference
    QJsonDocument json // NOLINT(*-unnecessary-value-param)
) {
    try {
        const auto jsonObj = json.toVariant().toJsonObject();
        const auto schemaJsonObj = jsonSchema.toVariant().toJsonObject();
        const QtJsonAdapter jsonSchemaAdapter(schemaJsonObj);
        SchemaParser parser;
        Schema schema;
        parser.populateSchema(jsonSchemaAdapter, schema);
        Validator validator(Validator::kStrongTypes);
        ValidationResults results;
        if (const QtJsonAdapter jsonAdapter(jsonObj);
            validator.validate(schema, jsonAdapter, &results)) {
            return std::nullopt;
        }
        return results;
    }
    catch (...) {
        return ValidationResults{};
    }
}

QJsonDocument serialization::toJson(Scene &scene) {
    QJsonArray points;
    QJsonArray geometry;

    // map the points first, then translate handles to ids for the geometry
    PointHandleToIdMap map{};
    for (const auto &entityPtr : scene.getEntities()) {
        Entity *e = entityPtr.get();
        const auto eId = static_cast<int>(e->getId());
        const auto eName = QString::fromStdString(e->getName());

        if (const auto pc = e->getComponent<PointComponent>()) {
            const auto handle = pc.value()->m_handle;
            points.append(
                QJsonObject{
                    {sp::id, eId},
                    {sp::name, eName},
                    {sp::position, vecToJson(scene.getPointRegistry().getPosition(handle))}
                }
            );
            map.emplace(handle, eId);
        }
    }

    for (const auto &entityPtr : scene.getEntities()) {
        Entity *e = entityPtr.get();
        const auto eId = static_cast<int>(e->getId());
        const auto eName = QString::fromStdString(e->getName());

        QJsonObject obj{
            {sp::id, eId},
            {sp::name, eName},
        };

        // skip the points this time
        if (e->getComponent<PointComponent>()) {
            continue;
        }

        if (const auto tg = e->getComponent<TorusComponent>()) {
            const auto t = e->getComponent<TransformComponent>().value();
            obj[sp::objectType] = st::torus;
            obj[sp::position] = vecToJson(t->getTranslation());
            obj[sp::rotation] = quatToJson(cadm::Quat::fromEuler(t->getRotation()));
            obj[sp::scale] = vecToJson(t->getScale());
            obj[sp::samples] = uint2ToJson(
                static_cast<int>(tg.value()->getMajorSegments()),
                static_cast<int>(tg.value()->getMinorSegments())
            );
            obj[sp::largeRadius] = tg.value()->getMajorRadius();
            obj[sp::smallRadius] = tg.value()->getMinorRadius();
        }
        else if (const auto bc = e->getComponent<BezierC0Component>()) {
            obj[sp::objectType] = st::bezierC0;
            obj[sp::controlPoints] = controlPointsToJson(bc.value()->getControlPoints(), map);
        }
        else if (const auto bc2 = e->getComponent<BezierC2Component>()) {
            obj[sp::objectType] = st::bezierC2;
            obj[sp::controlPoints] = controlPointsToJson(bc2.value()->getDeBoorPoints(), map);
        }
        else if (const auto ic = e->getComponent<InterpC2Component>()) {
            obj[sp::objectType] = st::interpolatedC2;
            obj[sp::controlPoints] = controlPointsToJson(ic.value()->getControlPoints(), map);
        }
        else if (const auto p0 = e->getComponent<PatchC0Component>()) {
            obj[sp::objectType] = st::bezierSurfaceC0;
            obj[sp::controlPoints] = controlPointsToJson(patchGridForJson(p0.value(), 1), map);
            obj[sp::size] = uint2ToJson(p0.value()->getCols(), patchJsonRows(p0.value(), 1));
            obj[sp::samples] = uint2ToJson(p0.value()->getGridDivisionsU(), p0.value()->getGridDivisionsV());
        }
        else if (const auto p2 = e->getComponent<PatchC2Component>()) {
            obj[sp::objectType] = st::bezierSurfaceC2;
            obj[sp::controlPoints] = controlPointsToJson(patchGridForJson(p2.value(), 3), map);
            obj[sp::size] = uint2ToJson(p2.value()->getCols(), patchJsonRows(p2.value(), 3));
            obj[sp::samples] = uint2ToJson(p2.value()->getGridDivisionsU(), p2.value()->getGridDivisionsV());
        }
        else {
            // no serializable geometry component
            continue;
        }

        geometry.append(obj);
    }

    return QJsonDocument(
        QJsonObject{
            {sp::points, points},
            {sp::geometry, geometry}
        }
    );
}

std::vector<Entity*> serialization::fromJson(Scene &scene, const QJsonDocument &json) {
    const auto root = json.object();

    std::vector<Entity*> result{};
    PointIdToHandleMap pointMap{};

    const auto pointsArr = root[sp::points].toArray();
    result.reserve(pointsArr.size());
    for (const auto &p : pointsArr) {
        result.push_back(pointFromJson(scene, p.toObject(), pointMap));
    }

    for (const auto &g : root[sp::geometry].toArray()) {
        if (Entity *e = entityFromJson(scene, g.toObject(), pointMap)) {
            result.push_back(e);
        }
    }

    return result;
}
