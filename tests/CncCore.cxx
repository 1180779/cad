//
// Created by Radosław Głasek on 18.10.2026
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cnc/HeightField.hxx"
#include "cnc/NcProgram.hxx"
#include "cnc/Simulator.hxx"

namespace {
    cnc::ParseResult parseText(
        std::string text,
        std::string name = "test.k06",
        std::optional<cnc::Tool> tool = std::nullopt
    ) {
        std::istringstream stream(std::move(text));
        return cnc::parseNcProgram(stream, std::move(name), tool);
    }

    /// @brief Safe move access. A missing move yields nullptr so a dependent
    /// check fails without aborting the rest of the test case.
    const cnc::Move *moveAt(const cnc::ParseResult &result, const std::size_t index) {
        if (!result.program || index >= result.program->moves.size()) {
            return nullptr;
        }
        return &result.program->moves[index];
    }

    /// @brief Safe duration access for the same reason as moveAt.
    double durationAt(const cnc::ParseResult &result, const std::size_t index) {
        const cnc::Move *move = moveAt(result, index);
        return move != nullptr
                   ? move->durationSeconds()
                   : std::numeric_limits<double>::quiet_NaN();
    }

    /// @brief Parse and report failure without aborting the test. Returning an
    /// empty optional lets the caller skip only the setup that requires a
    /// program while the remaining sections still run.
    std::optional<cnc::Program> parseChecked(
        std::string text,
        std::string name = "test.k06",
        std::optional<cnc::Tool> tool = std::nullopt
    ) {
        auto result = parseText(std::move(text), std::move(name), tool);
        CHECK(result.ok());
        if (!result.ok()) {
            return std::nullopt;
        }
        return std::move(result.program);
    }

    void appendChecked(
        cnc::Simulator &simulator,
        std::string text,
        std::string name = "test.k06"
    ) {
        if (auto program = parseChecked(std::move(text), std::move(name))) {
            simulator.appendProgram(std::move(program.value()));
        }
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

    bool hasWarning(const cnc::Snapshot &snapshot, const cnc::WarningKind kind) {
        return std::any_of(
            snapshot.warnings.begin(),
            snapshot.warnings.end(),
            [kind](const cnc::Warning &warning) {
                return warning.kind == kind;
            }
        );
    }

    /// @brief Test-only CPU oracle: replay the simulator's pending cutter moves
    /// into the reference height field.
    cnc::HeightField cpuCut(cnc::Simulator &simulator) {
        cnc::HeightField field(simulator.material());
        for (const cnc::Move &move : simulator.takeCutterMoves()) {
            field.cut(move, move.tool);
        }
        return field;
    }

    std::string modalProgram() {
        return
            "%G71\n"
            "N1G40G90\n"
            "N2S10000M03\n"
            "N3F15000\n"
            "N4G00Z300.000\n"
            "N5G01X0.000Y0.000Z0.000\n"
            "N6G01X10.000\n"
            "N7G01Y5.000\n"
            "N8M30\n";
    }

    std::string zigzagProgram() {
        return
            "%G71\n"
            "G00X-8.000Y-8.000Z5.000\n"
            "G01Z-2.000\n"
            "G01X8.000Y-8.000\n"
            "G01X8.000Y8.000\n"
            "G01X-8.000Y8.000\n"
            "G01X-8.000Y-8.000\n";
    }

    std::string straightProgram() {
        return
            "%G71\n"
            "G00X-8.000Y0.000Z2.000\n"
            "G01Z-3.000\n"
            "G01X8.000\n";
    }
}

TEST_CASE("material configuration supports a 1500x1500 grid", "[cnc][material]") {
    const cnc::MaterialConfig material{
        .width = 100.0f,
        .depth = 100.0f,
        .thickness = 20.0f,
        .subdivisionsX = 1500,
        .subdivisionsY = 1500,
    };
    const cnc::MaterialConfig normalized = material.normalized();

    SECTION("X subdivisions are preserved") {
        CHECK(normalized.subdivisionsX == 1500);
    }

    SECTION("Y subdivisions are preserved") {
        CHECK(normalized.subdivisionsY == 1500);
    }

    SECTION("X resolution includes the boundary samples") {
        CHECK(normalized.resolutionX() == 1501);
    }

    SECTION("Y resolution includes the boundary samples") {
        CHECK(normalized.resolutionY() == 1501);
    }
}

TEST_CASE("tool type and diameter are inferred from the file name", "[cnc][parser]") {
    SECTION("ball type") {
        const auto tool = cnc::toolFromFilename("part.k06");
        CHECK((tool && tool->type == cnc::ToolType::ball));
    }

    SECTION("ball diameter") {
        const auto tool = cnc::toolFromFilename("part.k06");
        CHECK((tool && tool->diameter == 6.0f));
    }

    SECTION("flat type") {
        const auto tool = cnc::toolFromFilename("roughing.f03");
        CHECK((tool && tool->type == cnc::ToolType::flat));
    }

    SECTION("flat diameter") {
        const auto tool = cnc::toolFromFilename("roughing.f03");
        CHECK((tool && tool->diameter == 3.0f));
    }

    SECTION("suffix before another extension") {
        const auto tool = cnc::toolFromFilename("model.k08.nc");
        CHECK((tool && tool->diameter == 8.0f));
    }

    SECTION("unrelated extension") {
        CHECK_FALSE(cnc::toolFromFilename("part.txt").has_value());
    }

    SECTION("zero diameter is rejected") {
        CHECK_FALSE(cnc::toolFromFilename("part.k00").has_value());
    }

    SECTION("malformed diameter is rejected") {
        CHECK_FALSE(cnc::toolFromFilename("part.kx").has_value());
    }

    SECTION("missing diameter is rejected") {
        CHECK_FALSE(cnc::toolFromFilename("part.k").has_value());
    }
}

TEST_CASE("parser handles modal coordinates and G00/G01", "[cnc][parser]") {
    const auto result = parseText(modalProgram());
    const auto &program = result.program;

    SECTION("parsing succeeds") {
        CHECK(result.ok());
    }

    SECTION("tool type") {
        CHECK((program && program->tool.type == cnc::ToolType::ball));
    }

    SECTION("tool diameter") {
        CHECK((program && program->tool.diameter == 6.0f));
    }

    SECTION("move count") {
        CHECK((program && program->moves.size() == 4));
    }

    SECTION("first move is rapid") {
        const cnc::Move *move = moveAt(result, 0);
        CHECK((move && move->type == cnc::MoveType::G00));
    }

    SECTION("first move starts at the origin") {
        const cnc::Move *move = moveAt(result, 0);
        CHECK((move && move->from == cadm::Vec3{}));
    }

    SECTION("first move ends at the safe height") {
        const cnc::Move *move = moveAt(result, 0);
        CHECK((move && move->to == cadm::Vec3(0, 0, 300)));
    }

    SECTION("second move is a cutting move") {
        const cnc::Move *move = moveAt(result, 1);
        CHECK((move && move->type == cnc::MoveType::G01));
    }

    SECTION("second move returns to the origin") {
        const cnc::Move *move = moveAt(result, 1);
        CHECK((move && move->to == cadm::Vec3(0, 0, 0)));
    }

    SECTION("modal X move") {
        const cnc::Move *move = moveAt(result, 2);
        CHECK((move && move->to == cadm::Vec3(10, 0, 0)));
    }

    SECTION("modal Y move") {
        const cnc::Move *move = moveAt(result, 3);
        CHECK((move && move->to == cadm::Vec3(10, 5, 0)));
    }

    SECTION("feed rate is carried by moves") {
        const cnc::Move *move = moveAt(result, 2);
        CHECK((move && move->feed == 15000.0f));
    }

    SECTION("rapid duration uses the rapid feed") {
        CHECK_THAT(
            durationAt(result, 0),
            Catch::Matchers::WithinAbs(0.9, 1e-6)
        );
    }

    SECTION("cutting duration uses the cutting feed") {
        CHECK_THAT(
            durationAt(result, 1),
            Catch::Matchers::WithinAbs(1.2, 1e-6)
        );
    }
}

TEST_CASE("parser applies inch units from G70", "[cnc][parser]") {
    const auto result = parseText("%G70\nG01X1.000Y1.000Z-0.500\n");
    const auto &program = result.program;

    SECTION("parsing succeeds") {
        CHECK(result.ok());
    }

    SECTION("inch input is recorded") {
        CHECK((program && program->unitType == cnc::UnitType::Imperial));
    }

    SECTION("X is converted to millimetres") {
        const cnc::Move *move = moveAt(result, 0);
        CHECK((move && std::abs(move->to.x - 25.4f) < 1e-4f));
    }

    SECTION("Y is converted to millimetres") {
        const cnc::Move *move = moveAt(result, 0);
        CHECK((move && std::abs(move->to.y - 25.4f) < 1e-4f));
    }

    SECTION("Z is converted to millimetres") {
        const cnc::Move *move = moveAt(result, 0);
        CHECK((move && std::abs(move->to.z + 12.7f) < 1e-4f));
    }
}

TEST_CASE("parser ignores comments and tolerates lowercase", "[cnc][parser]") {
    const auto result = parseText(
        "%G71\n"
        "// a line comment\n"
        "(parenthesised comment)\n"
        "g01 x10 y0\n"
        "g01 y10 ; trailing comment\n"
    );
    const auto &program = result.program;

    SECTION("parsing succeeds") {
        CHECK(result.ok());
    }

    SECTION("comment-only lines are skipped") {
        CHECK((program && program->moves.size() == 2));
    }

    SECTION("lowercase commands are accepted") {
        const cnc::Move *move = moveAt(result, 1);
        CHECK((move && move->to == cadm::Vec3(10, 10, 0)));
    }
}

TEST_CASE("parser matches nested inline comment delimiters", "[cnc][parser]") {
    const std::vector<std::string> lines{
        "G01/* comment */X10",
        "G01(outer (inner) outer)X10",
        "G01/* outer /* inner */ outer */X10",
        "G01(outer /* inner ) */ outer)X10",
        "G01/* outer (inner */) outer */X10",
        "G01/* ; // ignored */X10",
        "G01(; // ignored)X10",
        "G01/**/()X10// ignored",
        "G01X10; ignored /*",
        "G01X10/* unterminated",
        "G01X10(unterminated",
        "G01X10/* partial ending *",
    };
    for (const auto& line : lines) {
        CAPTURE(line);
        const auto result = parseText(line + "\n");
        REQUIRE(result.ok());
        REQUIRE(result.program->moves.size() == 1);
        CHECK(result.program->moves.front().to.x == 10.0f);
    }
}

TEST_CASE("parser warns about unsupported arc and incremental modes", "[cnc][parser]") {
    SECTION("arc motion warning") {
        auto result = parseText("%G71\nG02X10.000Y0.000\n");
        // WError is the default; opt out so the warning remains non-fatal.
        result.flags = {};
        CHECK(result.ok());
        CHECK(std::any_of(
            result.diagnostics.begin(),
            result.diagnostics.end(),
            [](const cnc::Diagnostic &diagnostic) {
                return diagnostic.type == cnc::DiagnosticMessageType::Warning
                    && diagnostic.message.find("Arc motion") != std::string::npos;
            }
        ));
    }

    SECTION("incremental mode warning") {
        auto result = parseText("%G71\nG91\nG01X10.000\n");
        result.flags = {};
        CHECK(result.ok());
        CHECK(std::any_of(
            result.diagnostics.begin(),
            result.diagnostics.end(),
            [](const cnc::Diagnostic &diagnostic) {
                return diagnostic.type == cnc::DiagnosticMessageType::Warning
                    && diagnostic.message.find("Incremental") != std::string::npos;
            }
        ));
    }
}

TEST_CASE("parser reports malformed and empty programs", "[cnc][parser]") {
    SECTION("malformed value is an error") {
        const auto result = parseText("%G71\nN1G01X\n");
        CHECK_FALSE(result.ok());
        CHECK_FALSE(result.diagnostics.empty());
        CHECK((
            !result.diagnostics.empty()
            && result.diagnostics.front().type == cnc::DiagnosticMessageType::Error
        ));
    }

    SECTION("a program without motion is an error") {
        const auto result = parseText("%G71\nN1G40G90\n");
        CHECK_FALSE(result.ok());
    }
}

TEST_CASE("tool override takes precedence over the file name", "[cnc][parser]") {
    const auto result = parseText(
        "%G71\nG01X10\n",
        "part.k06",
        cnc::Tool{.type = cnc::ToolType::flat, .diameter = 2.0f}
    );

    SECTION("parsing succeeds") {
        CHECK(result.ok());
    }

    SECTION("overridden type") {
        CHECK((result.program && result.program->tool.type == cnc::ToolType::flat));
    }

    SECTION("overridden diameter") {
        CHECK((result.program && result.program->tool.diameter == 2.0f));
    }
}

TEST_CASE("flat cutter removes a flat strip and clamps at the base", "[cnc][heightfield]") {
    cnc::HeightField field(smallStock());
    const cnc::Tool tool{.type = cnc::ToolType::flat, .diameter = 4.0f};
    const cnc::Move move{
        .from = {-10, 0, -5},
        .to = {10, 0, -5},
        .type = cnc::MoveType::G01,
        .feed = 15000.0f,
        .tool = tool,
    };
    const auto result = field.cut(move, tool);

    SECTION("material is removed") {
        CHECK(result.removedVolume > 0.0);
    }

    SECTION("cells are touched") {
        CHECK(result.touchedCells > 0);
    }

    SECTION("height on the centre line") {
        CHECK_THAT(field.sampleHeight(0, 0), Catch::Matchers::WithinAbs(-5.0f, 1e-5f));
    }

    SECTION("height inside the cutter radius") {
        CHECK_THAT(field.sampleHeight(0, 1), Catch::Matchers::WithinAbs(-5.0f, 1e-5f));
    }

    SECTION("height outside the cutter radius") {
        CHECK_THAT(field.sampleHeight(0, 3), Catch::Matchers::WithinAbs(0.0f, 1e-5f));
    }

    SECTION("cutting below the base clamps at the base") {
        const cnc::Move tooDeep{
            .from = {-5, 0, -20},
            .to = {5, 0, -20},
            .type = cnc::MoveType::G01,
            .feed = 15000.0f,
            .tool = tool,
        };
        field.cut(tooDeep, tool);
        CHECK_THAT(field.sampleHeight(0, 0), Catch::Matchers::WithinAbs(-10.0f, 1e-5f));
    }
}

TEST_CASE("ball cutter produces a rounded profile", "[cnc][heightfield]") {
    cnc::HeightField field(smallStock());
    const cnc::Tool tool{.type = cnc::ToolType::ball, .diameter = 4.0f};
    const cnc::Move move{
        .from = {-10, 0, -5},
        .to = {10, 0, -5},
        .type = cnc::MoveType::G01,
        .feed = 15000.0f,
        .tool = tool,
    };
    field.cut(move, tool);

    constexpr float radius = 2.0f;
    const float expectedAtOne = -5.0f + radius - std::sqrt(radius * radius - 1.0f);

    SECTION("height at the tool centre") {
        CHECK_THAT(field.sampleHeight(0, 0), Catch::Matchers::WithinAbs(-5.0f, 1e-5f));
    }

    SECTION("height one millimetre off the centre") {
        CHECK_THAT(
            field.sampleHeight(0, 1),
            Catch::Matchers::WithinAbs(expectedAtOne, 1e-4f)
        );
    }

    SECTION("height outside the ball radius") {
        CHECK_THAT(field.sampleHeight(0, 3), Catch::Matchers::WithinAbs(0.0f, 1e-5f));
    }
}

TEST_CASE("height field tracks dirty rectangles", "[cnc][heightfield]") {
    cnc::HeightField field(smallStock());
    const auto initial = field.takeDirty();

    SECTION("initial dirty rectangle is valid") {
        CHECK(initial.valid());
    }

    SECTION("initial dirty rectangle starts at X zero") {
        CHECK(initial.minX == 0);
    }

    SECTION("initial dirty rectangle starts at Y zero") {
        CHECK(initial.minY == 0);
    }

    SECTION("a cut produces a bounded dirty rectangle") {
        const cnc::Tool tool{.type = cnc::ToolType::flat, .diameter = 2.0f};
        field.cut(
            cnc::Move{
                .from = {-2, 2, -1},
                .to = {2, 2, -1},
                .type = cnc::MoveType::G01,
                .feed = 15000.0f,
                .tool = tool,
            },
            tool
        );
        const auto dirty = field.takeDirty();
        CHECK(dirty.valid());
        CHECK((dirty.valid() && dirty.minX >= 7));
        CHECK((dirty.valid() && dirty.maxX <= 13));
        CHECK((dirty.valid() && dirty.minY >= 11));
        CHECK((dirty.valid() && dirty.maxY <= 13));
    }

    SECTION("dirty data is consumed on read") {
        const cnc::Tool tool{.type = cnc::ToolType::flat, .diameter = 2.0f};
        field.cut(
            cnc::Move{
                .from = {-2, 2, -1},
                .to = {2, 2, -1},
                .type = cnc::MoveType::G01,
                .feed = 15000.0f,
                .tool = tool,
            },
            tool
        );
        (void) field.takeDirty();
        CHECK_FALSE(field.takeDirty().valid());
    }
}

TEST_CASE("vertical plunge is reported", "[cnc][simulator]") {
    cnc::Simulator simulator(smallStock());
    appendChecked(simulator, "%G71\nG01Z-2.000\n");
    simulator.finishNow();
    const auto snapshot = simulator.makeSnapshot();

    SECTION("vertical plunge warning") {
        CHECK(hasWarning(snapshot, cnc::WarningKind::verticalPlunge));
    }

    SECTION("simulation is finished") {
        CHECK(snapshot.finished);
    }
}

TEST_CASE("cutting below the stock base is reported", "[cnc][simulator]") {
    cnc::Simulator simulator(smallStock());
    appendChecked(simulator, "%G71\nG01X2.000Z-15.000\n");
    simulator.finishNow();
    const auto snapshot = simulator.makeSnapshot();
    const auto reference = cpuCut(simulator);

    SECTION("too-deep warning") {
        CHECK(hasWarning(snapshot, cnc::WarningKind::tooDeep));
    }

    SECTION("material is clamped at the base") {
        CHECK_THAT(
            reference.sampleHeight(2, 0),
            Catch::Matchers::WithinAbs(-10.0f, 1e-5f)
        );
    }
}

TEST_CASE("ball shank collision is reported", "[cnc][simulator]") {
    cnc::Simulator simulator(smallStock());
    appendChecked(simulator, "%G71\nG01X5.000Z-5.000\n", "small.k02");
    simulator.finishNow();
    const auto snapshot = simulator.makeSnapshot();

    SECTION("non-cutting-part warning") {
        CHECK(hasWarning(snapshot, cnc::WarningKind::nonCuttingPart));
    }
}

TEST_CASE("multiple programs keep previously removed material", "[cnc][simulator]") {
    cnc::Simulator simulator(smallStock());
    appendChecked(simulator, "%G71\nG00X-5.000\nG01Z-2.000\n");
    appendChecked(simulator, "%G71\nG00X5.000\nG01Z-2.000\n");
    simulator.finishNow();
    const auto snapshot = simulator.makeSnapshot();
    const auto reference = cpuCut(simulator);

    SECTION("first program remains milled") {
        CHECK_THAT(
            reference.sampleHeight(-5, 0),
            Catch::Matchers::WithinAbs(-2.0f, 1e-5f)
        );
    }

    SECTION("second program is milled into the same stock") {
        CHECK_THAT(
            reference.sampleHeight(5, 0),
            Catch::Matchers::WithinAbs(-2.0f, 1e-5f)
        );
    }

    SECTION("both programs are retained") {
        CHECK(simulator.programs().size() == 2);
    }

    SECTION("the snapshot reports both programs' moves") {
        CHECK(snapshot.instructionCount == 4);
    }
}

TEST_CASE("finish now matches stepped simulation", "[cnc][simulator]") {
    cnc::Simulator stepped(smallStock());
    appendChecked(stepped, zigzagProgram());

    cnc::Simulator instant(smallStock());
    appendChecked(instant, zigzagProgram());

    stepped.advance(1e9);
    instant.finishNow();

    const auto steppedField = cpuCut(stepped);
    const auto instantField = cpuCut(instant);

    SECTION("the grids have the same size") {
        CHECK(
            steppedField.heights().size()
            == instantField.heights().size()
        );
    }

    SECTION("every height matches") {
        const auto &a = steppedField.heights();
        const auto &b = instantField.heights();
        CHECK(a.size() == b.size());
        const std::size_t common = std::min(a.size(), b.size());
        for (std::size_t i = 0; i < common; ++i) {
            CHECK_THAT(a[i], Catch::Matchers::WithinAbs(b[i], 1e-4f));
        }
    }

    SECTION("both simulations finish") {
        CHECK(stepped.finished());
        CHECK(instant.finished());
    }
}

TEST_CASE("simulation result is independent of the step size", "[cnc][simulator]") {
    cnc::Simulator fine(smallStock());
    appendChecked(fine, straightProgram());

    cnc::Simulator coarse(smallStock());
    appendChecked(coarse, straightProgram());

    for (int i = 0; i < 20; ++i) {
        fine.advance(0.5);
    }
    coarse.advance(10.0);

    const auto fineField = cpuCut(fine);
    const auto coarseField = cpuCut(coarse);

    SECTION("the grids have the same size") {
        CHECK(
            fineField.heights().size()
            == coarseField.heights().size()
        );
    }

    SECTION("every height matches") {
        const auto &a = fineField.heights();
        const auto &b = coarseField.heights();
        CHECK(a.size() == b.size());
        const std::size_t common = std::min(a.size(), b.size());
        for (std::size_t i = 0; i < common; ++i) {
            CHECK_THAT(a[i], Catch::Matchers::WithinAbs(b[i], 1e-4f));
        }
    }

    SECTION("both simulations finish") {
        CHECK(fine.finished());
        CHECK(coarse.finished());
    }
}

TEST_CASE("fixed-step advancement is independent of host frame jitter", "[cnc][simulator]") {
    constexpr double fixedStep = 0.01;
    constexpr double total = 0.08;

    cnc::Simulator steady(smallStock());
    appendChecked(steady, straightProgram());

    cnc::Simulator burst(smallStock());
    appendChecked(burst, straightProgram());

    cnc::Simulator jitter(smallStock());
    appendChecked(jitter, straightProgram());

    cnc::Simulator direct(smallStock());
    appendChecked(direct, straightProgram());

    for (int i = 0; i < 80; ++i) {
        steady.advanceFixed(0.001, fixedStep);
    }
    for (int i = 0; i < 4; ++i) {
        burst.advanceFixed(0.020, fixedStep);
    }
    for (int i = 0; i < 20; ++i) {
        jitter.advanceFixed(0.001, fixedStep);
        jitter.advanceFixed(0.003, fixedStep);
    }
    direct.advance(total);

    const auto steadyField = cpuCut(steady);
    const auto burstField = cpuCut(burst);
    const auto jitterField = cpuCut(jitter);
    const auto directField = cpuCut(direct);

    const auto fieldsMatch = [](const cnc::HeightField &a, const cnc::HeightField &b) {
        CHECK(a.heights().size() == b.heights().size());
        const std::size_t common = std::min(a.heights().size(), b.heights().size());
        for (std::size_t i = 0; i < common; ++i) {
            CHECK_THAT(
                a.heights()[i],
                Catch::Matchers::WithinAbs(b.heights()[i], 1e-4f)
            );
        }
    };

    SECTION("small host ticks match large host ticks") {
        fieldsMatch(steadyField, burstField);
    }

    SECTION("jittered host ticks match uniform host ticks") {
        fieldsMatch(steadyField, jitterField);
    }

    SECTION("fixed-step cuts match a single direct delta") {
        fieldsMatch(steadyField, directField);
    }

    SECTION("the fixed-step result is not yet finished") {
        CHECK_FALSE(steady.finished());
    }
}

TEST_CASE("snapshots expose progress, paths and queued cutter moves", "[cnc][simulator]") {
    cnc::Simulator simulator(smallStock());
    appendChecked(simulator, "%G71\nG01X10.000Z-2.000\n");

    const auto initial = simulator.makeSnapshot();
    simulator.finishNow();
    const auto finished = simulator.makeSnapshot();

    SECTION("programs are present") {
        CHECK(initial.hasPrograms);
    }

    SECTION("the move count is reported") {
        CHECK(initial.instructionCount == 1);
    }

    SECTION("the material configuration is reported") {
        CHECK(initial.material.width == 20.0f);
    }

    SECTION("the initial snapshot is not finished") {
        CHECK_FALSE(initial.finished);
    }

    SECTION("the path snapshot is shared") {
        CHECK(initial.moves != nullptr);
    }

    SECTION("the path snapshot contains the move") {
        CHECK((initial.moves && initial.moves->size() == 1));
    }

    SECTION("the simulation is finished") {
        CHECK(finished.finished);
    }

    SECTION("progress reaches one") {
        CHECK_THAT(finished.progress, Catch::Matchers::WithinAbs(1.0f, 1e-6f));
    }

    SECTION("the revision advances") {
        CHECK(finished.revision > initial.revision);
    }

    SECTION("cutter moves are queued for the GPU") {
        CHECK(simulator.pendingCutterMoveCount() == 1);
    }

    SECTION("cutter moves can be drained") {
        const auto moves = simulator.takeCutterMoves();
        CHECK(moves.size() == 1);
        CHECK(simulator.pendingCutterMoveCount() == 0);
    }
}
