//
// Created by Radosław Głasek on 18.10.2026
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLVersionFunctionsFactory>
#include <QSurfaceFormat>

#include "cnc/HeightField.hxx"
#include "cnc/NcProgram.hxx"
#include "cnc_gpu/GpuHeightMap.hxx"

namespace {
    QGuiApplication &guiApplication() {
        static int argc = 1;
        static char name[] = "cad_cnc_gpu_tests";
        static char *argv[] = {name, nullptr};
        static QGuiApplication application(argc, argv);
        return application;
    }

    cnc::MaterialConfig smallStock() {
        return {
            .width = 20.0f,
            .depth = 20.0f,
            .thickness = 10.0f,
            .subdivisionsX = 20,
            .subdivisionsY = 20,
        };
    }

    cnc::Move flatMove(const cadm::Vec3 from, const cadm::Vec3 to) {
        return {
            .from = from,
            .to = to,
            .type = cnc::MoveType::G01,
            .feed = 15000.0f,
            .tool = {.type = cnc::ToolType::flat, .diameter = 4.0f},
        };
    }

    cnc::Move ballMove(const cadm::Vec3 from, const cadm::Vec3 to) {
        return {
            .from = from,
            .to = to,
            .type = cnc::MoveType::G01,
            .feed = 15000.0f,
            .tool = {.type = cnc::ToolType::ball, .diameter = 4.0f},
        };
    }

    std::size_t cuttingMoveCount(const std::vector<cnc::Move> &moves) {
        return static_cast<std::size_t>(std::count_if(
            moves.begin(),
            moves.end(),
            [](const cnc::Move &move) {
                return move.type != cnc::MoveType::G00 && move.tool.radius() > 0.0f;
            }
        ));
    }

    void checkAgainstCpu(
        cnc::gpu::GpuHeightMap &map,
        const cnc::MaterialConfig &material,
        const std::vector<cnc::Move> &moves,
        const std::size_t batchSize,
        const std::string &scenario
    ) {
        INFO("scenario: " << scenario);
        map.setBatchSize(batchSize);
        map.reset();

        cnc::HeightField reference(material);
        for (const cnc::Move &move : moves) {
            if (move.type != cnc::MoveType::G00) {
                reference.cut(move, move.tool);
            }
        }

        CHECK(map.applyAll(moves) == cuttingMoveCount(moves));
        const std::vector<float> gpuHeights = map.readHeights();
        CHECK(gpuHeights.size() == reference.heights().size());

        const std::size_t common = std::min(gpuHeights.size(), reference.heights().size());
        for (std::size_t i = 0; i < common; ++i) {
            CHECK_THAT(
                gpuHeights[i],
                Catch::Matchers::WithinAbs(reference.heights()[i], 1e-3f)
            );
        }
    }

    void checkLargeAgainstCpu(
        cnc::gpu::GpuHeightMap &map,
        const cnc::MaterialConfig &material,
        const std::vector<cnc::Move> &moves,
        const std::size_t batchSize,
        const std::string &scenario
    ) {
        INFO("large scenario: " << scenario);
        map.setBatchSize(batchSize);
        map.reset();

        cnc::HeightField reference(material);
        for (const cnc::Move &move : moves) {
            if (move.type != cnc::MoveType::G00) {
                reference.cut(move, move.tool);
            }
        }

        CHECK(map.applyAll(moves) == cuttingMoveCount(moves));
        const std::vector<float> gpuHeights = map.readHeights();
        CHECK(gpuHeights.size() == reference.heights().size());

        const std::size_t common = std::min(gpuHeights.size(), reference.heights().size());
        std::size_t mismatches = 0;
        float maximumError = 0.0f;
        for (std::size_t i = 0; i < common; ++i) {
            const float error = std::abs(gpuHeights[i] - reference.heights()[i]);
            if (error > 1e-3f) {
                ++mismatches;
                maximumError = std::max(maximumError, error);
            }
        }
        CHECK(mismatches == 0);
        CHECK(maximumError < 1e-3f);
    }
}

TEST_CASE("GPU height map matches the CPU reference", "[cnc][gpu]") {
    guiApplication();

    QSurfaceFormat format;
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);

    QOpenGLContext context;
    context.setFormat(format);
    CHECK(context.create());
    if (!context.isValid()) {
        return;
    }

    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    CHECK(surface.isValid());
    if (!surface.isValid()) {
        return;
    }

    CHECK(context.makeCurrent(&surface));
    if (QOpenGLContext::currentContext() != &context) {
        return;
    }

    const auto *gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(&context);
    CHECK(gl != nullptr);
    if (gl == nullptr) {
        context.doneCurrent();
        return;
    }

    const cnc::MaterialConfig material = smallStock();
    cnc::gpu::GpuHeightMap map;
    std::string error;
    CHECK(map.initialize(material, CNC_GPU_SHADER_DIR, &error));
    if (!map.initialized()) {
        INFO("GPU height map initialization failed: " << error);
        context.doneCurrent();
        return;
    }

    checkAgainstCpu(
        map,
        material,
        {flatMove({-10, 0, -5}, {10, 0, -5})},
        64,
        "flat horizontal move"
    );

    checkAgainstCpu(
        map,
        material,
        {ballMove({-10, 0, -5}, {10, 0, -5})},
        64,
        "ball horizontal move"
    );

    checkAgainstCpu(
        map,
        material,
        {flatMove({0, 0, 0}, {0, 0, -5})},
        1,
        "vertical plunge"
    );

    checkAgainstCpu(
        map,
        material,
        {
            flatMove({-8, -8, -2}, {8, -8, -2}),
            ballMove({8, -8, -3}, {8, 8, -3}),
            flatMove({8, 8, -1}, {-8, 8, -1}),
        },
        2,
        "mixed batch"
    );

    checkAgainstCpu(
        map,
        material,
        {flatMove({-5, 0, -20}, {5, 0, -20})},
        64,
        "base clamp"
    );

    const cnc::MaterialConfig largeMaterial{
        .width = 100.0f,
        .depth = 100.0f,
        .thickness = 20.0f,
        .subdivisionsX = 1500,
        .subdivisionsY = 1500,
    };
    cnc::gpu::GpuHeightMap largeMap;
    std::string largeError;
    CHECK(largeMap.initialize(largeMaterial, CNC_GPU_SHADER_DIR, &largeError));
    if (largeMap.initialized()) {
        checkLargeAgainstCpu(
            largeMap,
            largeMaterial,
            {
                flatMove({-45, -45, -2}, {45, -45, -2}),
                flatMove({45, -45, -2}, {45, 45, -2}),
                flatMove({45, 45, -2}, {-45, 45, -2}),
                flatMove({-45, 45, -2}, {-45, -45, -2}),
                ballMove({-30, -30, -4}, {30, -30, -4}),
                ballMove({30, -30, -4}, {30, 30, -4}),
                ballMove({30, 30, -4}, {-30, 30, -4}),
                ballMove({-30, 30, -4}, {-30, -30, -4}),
            },
            64,
            "1500x1500 stock"
        );
        largeMap.shutdown();
    }

    map.shutdown();
    context.doneCurrent();
}
