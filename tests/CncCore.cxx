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

#include "cnc/NcProgram.hxx"
#include "cnc/Simulator.hxx"

namespace {
    cnc::ParseResult parseText(
        std::string text,
        std::string name = "test.k06",
        std::optional<cnc::Tool> tool = std::nullopt
    ) {
        std::istringstream stream(std::move(text));
        cnc::ParseResult result = cnc::parseNcProgram(stream, std::move(name), tool);
        // Tests want warning-only programs to be considered parsed; errors
        // still make ok() return false.
        result.flags = static_cast<cnc::ParseResultFlags>(0);
        return result;
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
        const std::optional<cnc::Tool> tool = std::nullopt
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
        return std::ranges::any_of(snapshot.warnings,
                                   [kind](const cnc::Warning &warning) {
                                       return warning.kind == kind;
                                   }
        );
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
}

TEST_CASE("material configuration supports a 1500x1500 grid", "[cnc][material]") {
    constexpr cnc::MaterialConfig material{
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

TEST_CASE("programs start with a safe approach above the stock", "[cnc][simulator]") {
    cnc::MaterialConfig material = smallStock();
    material.topZ = 66.0f;
    material.thickness = 50.0f;

    cnc::Simulator simulator(material);
    appendChecked(
        simulator,
        "%G71\nG01X0.000Y0.000Z66.000\nG01X0.000Y0.000Z60.000\n"
    );
    const auto snapshot = simulator.makeSnapshot();

    SECTION("the initial tool position is above the stock top") {
        CHECK(snapshot.toolPosition.z > material.topZ);
    }

    SECTION("the first path move is a rapid approach") {
        CHECK((snapshot.moves && !snapshot.moves->empty()
            && snapshot.moves->front().type == cnc::MoveType::G00));
    }

    SECTION("the approach ends at the programmed safe height") {
        CHECK((snapshot.moves && !snapshot.moves->empty()
            && std::abs(snapshot.moves->front().to.z - material.topZ) < 1e-4f));
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

TEST_CASE("ball shank collision is reported", "[cnc][simulator]") {
    cnc::Simulator simulator(smallStock());
    appendChecked(simulator, "%G71\nG01X5.000Z-5.000\n", "small.k02");
    simulator.finishNow();
    const auto snapshot = simulator.makeSnapshot();

    SECTION("non-cutting-part warning") {
        CHECK(hasWarning(snapshot, cnc::WarningKind::nonCuttingPart));
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
        CHECK(initial.instructionCount == 2);
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
        CHECK((initial.moves && initial.moves->size() == 2));
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
