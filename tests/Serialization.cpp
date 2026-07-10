#include <catch2/catch_test_macros.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <cmath>
#include <fstream>
#include <sstream>

#include "Scene.hpp"
#include "serialization/Serialization.hxx"

namespace {
    QJsonDocument readJsonFile(const std::string &path) {
        std::ifstream file(path);
        REQUIRE(file.is_open());
        std::ostringstream ss;
        ss << file.rdbuf();
        return QJsonDocument::fromJson(QByteArray::fromStdString(ss.str()));
    }

    /// @brief Deep-equal that tolerates float/double rounding
    bool jsonApproxEqual(const QJsonValue &a, const QJsonValue &b, const double eps = 1e-5) {
        if (a.type() != b.type()) {
            return false;
        }
        switch (a.type()) {
        case QJsonValue::Double:
            return std::abs(a.toDouble() - b.toDouble()) <= eps;
        case QJsonValue::Object: {
            const auto oa = a.toObject();
            const auto ob = b.toObject();
            auto keysA = oa.keys();
            auto keysB = ob.keys();
            keysA.sort();
            keysB.sort();
            if (keysA != keysB) {
                return false;
            }
            // quaternions have a sign ambiguity (q and -q are the same rotation)
            if (keysA == QList<QString>{"w", "x", "y", "z"}) {
                const auto matches = [&](const int sign) {
                    for (const auto &key : keysA) {
                        if (std::abs(oa.value(key).toDouble() - sign * ob.value(key).toDouble()) > eps) {
                            return false;
                        }
                    }
                    return true;
                };
                return matches(1) || matches(-1);
            }
            for (const auto &key : keysA) {
                if (!jsonApproxEqual(oa.value(key), ob.value(key), eps)) {
                    return false;
                }
            }
            return true;
        }
        case QJsonValue::Array: {
            const auto aa = a.toArray();
            const auto ba = b.toArray();
            if (aa.size() != ba.size()) {
                return false;
            }
            for (int i = 0; i < aa.size(); ++i) {
                if (!jsonApproxEqual(aa[i], ba[i], eps)) {
                    return false;
                }
            }
            return true;
        }
        default: {
            if (a != b) {
                return false;
            }
            return true;
        }
        }
    }
}

TEST_CASE("small hand-built scene", "[serialization]") {
    const QJsonDocument input = QJsonDocument::fromJson(
        R"({
        "points": [
            { "id": 1, "name": "Point 1", "position": { "x": 0, "y": 0, "z": 0 } },
            { "id": 2, "name": "Point 2", "position": { "x": 1, "y": 0, "z": 0 } },
            { "id": 3, "name": "Point 3", "position": { "x": 1, "y": 1, "z": 0 } }
        ],
        "geometry": [
            {
                "id": 4,
                "objectType": "bezierC0",
                "name": "BezierC0 1",
                "controlPoints": [ { "id": 1 }, { "id": 2 }, { "id": 3 } ]
            }
        ]
    })"
    );

    Scene scene;
    serialization::fromJson(scene, input);
    const QJsonDocument output = serialization::toJson(scene);

    REQUIRE(jsonApproxEqual(input.object(), output.object()));
}

TEST_CASE("example scene", "[serialization]") {
    const QJsonDocument input = readJsonFile(std::string(CAD_FORMAT_DIR) + "/example_scene.json");

    Scene sceneA;
    serialization::fromJson(sceneA, input);
    const QJsonDocument firstSave = serialization::toJson(sceneA);

    Scene sceneB;
    serialization::fromJson(sceneB, firstSave);
    const QJsonDocument secondSave = serialization::toJson(sceneB);

    REQUIRE(jsonApproxEqual(input.object(), firstSave.object()));
    REQUIRE(jsonApproxEqual(firstSave.object(), secondSave.object()));

    const auto schema = readJsonFile(std::string(CAD_FORMAT_DIR) + "/schema.json");
    REQUIRE_FALSE(serialization::validateJson(schema, firstSave).has_value());
}

TEST_CASE("validateJson", "[serialization]") {
    const auto schema = readJsonFile(std::string(CAD_FORMAT_DIR) + "/schema.json");

    SECTION("accepts the example scene") {
        const auto doc = readJsonFile(std::string(CAD_FORMAT_DIR) + "/example_scene.json");
        REQUIRE_FALSE(serialization::validateJson(schema, doc).has_value());
    }

    SECTION("accepts a valid document") {
        const QJsonDocument doc = QJsonDocument::fromJson(
            R"({
            "points": [ { "id": 1, "name": "Point 1", "position": { "x": 0, "y": 0, "z": 0 } } ],
            "geometry": []
        })"
        );
        REQUIRE_FALSE(serialization::validateJson(schema, doc).has_value());
    }

    SECTION("rejects a document with an unknown top-level property") {
        const QJsonDocument doc = QJsonDocument::fromJson(
            R"({
            "points": [],
            "geometry": [],
            "unknownField": 42
        })"
        );
        REQUIRE(serialization::validateJson(schema, doc).has_value());
    }

    SECTION("rejects a point missing its required position") {
        const QJsonDocument doc = QJsonDocument::fromJson(
            R"({
            "points": [ { "id": 1, "name": "Point 1" } ],
            "geometry": []
        })"
        );
        REQUIRE(serialization::validateJson(schema, doc).has_value());
    }
}
