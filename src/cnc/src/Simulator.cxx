//
// Created by Radosław Głasek on 18.10.2026
//

#include "cnc/program/Tool.hxx"
#include "cnc/program/Move.hxx"
#include "cnc/Simulator.hxx"
#include "cnc/CompactMoves.hxx"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>

namespace cnc {
namespace {
std::string formatNumber(const double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

bool containsXY(const Material& material, const float x, const float y) {
    const float halfWidth = material.width * 0.5f;
    const float halfDepth = material.depth * 0.5f;
    return x >= material.centerX - halfWidth &&
           x <= material.centerX + halfWidth &&
           y >= material.centerY - halfDepth &&
           y <= material.centerY + halfDepth;
}
}

std::string_view warningKindName(const WarningKind kind) {
    switch (kind) {
    case WarningKind::nonCuttingPart:
        return "non-cutting part";
    case WarningKind::tooDeep:
        return "too deep";
    case WarningKind::verticalPlunge:
        return "vertical plunge";
    }
    return "unknown";
}

Simulator::Simulator(const Material& material)
    : m_material(material.normalized()) {
    rebuildPathSnapshot();
    resetTimeline();
}

void Simulator::setMaterial(const Material& material) {
    m_material = material.normalized();
    rebuildPathSnapshot();
    resetTimeline();
}

void Simulator::resetMaterial() {
    resetTimeline();
}

void Simulator::appendProgram(Program program) {
    if (program.moves.empty()) {
        m_programs.push_back(std::move(program));
        return;
    }

    const bool wasEmpty = m_moves.empty();
    const std::size_t programIndex = m_programs.size();
    for (Move& move : program.moves) {
        move.programIndex = programIndex;
        move.tool = program.tool;
    }

    m_moves.insert(
        m_moves.end(),
        std::make_move_iterator(program.moves.begin()),
        std::make_move_iterator(program.moves.end())
        );
    program.moves.clear();
    m_programs.push_back(std::move(program));

    rebuildPathSnapshot();
    m_finished = false;
    if (wasEmpty) {
        resetTimeline();
    }
}

void Simulator::clearPrograms() {
    m_moves.clear();
    m_programs.clear();
    rebuildPathSnapshot();
    resetTimeline();
}

void Simulator::play() {
    if (!m_moves.empty() && !m_finished) {
        m_playing = true;
    }
}

void Simulator::pause() {
    m_playing = false;
}

void Simulator::setPlaying(const bool playing) {
    if (playing) {
        play();
    } else {
        pause();
    }
}

bool Simulator::playing() const {
    return m_playing;
}

bool Simulator::finished() const {
    return m_finished;
}

void Simulator::resetTimeline() {
    ++m_timeline;
    m_moveIndex = 0;
    m_moveElapsed = 0.0;
    m_elapsed = 0.0;
    m_fixedStepRemainder = 0.0;
    m_playing = false;
    m_finished = m_moves.empty();
    m_warnings.clear();
    m_warningKeys.clear();
    m_suppressedWarnings = 0;
    m_pendingCutterMoves.clear();
    m_toolPosition = m_moves.empty() ? cadm::Vec3{} : m_moves.front().from;
}

void Simulator::rebuildPathSnapshot() {
    m_pathSnapshot = std::make_shared<const ToolPath>(m_moves);
    m_totalDuration = 0.0;
    for (const Move& move : m_moves) {
        m_totalDuration += move.durationSeconds();
    }
}

void Simulator::advance(const double simulatedSeconds) {
    if (simulatedSeconds <= 0.0 || m_moves.empty() || m_finished) {
        return;
    }

    double remaining = simulatedSeconds;
    while (m_moveIndex < m_moves.size()) {
        const Move& move = m_moves[m_moveIndex];
        const double moveDuration = move.durationSeconds();

        if (moveDuration <= gc_epsilon) {
            applyMove(move, m_moveIndex);
            ++m_moveIndex;
            m_moveElapsed = 0.0;
            continue;
        }

        if (remaining <= 0.0) {
            break;
        }

        const double remainingMoveDuration = moveDuration - m_moveElapsed;
        const double stepAdvancementDuration = std::min(remaining, remainingMoveDuration);
        const double t0 = m_moveElapsed;
        const double t1 = t0 + stepAdvancementDuration;

        if (stepAdvancementDuration > 0.0) {
            Move portion = move;
            portion.from = move.from.lerp(move.to, static_cast<cadm::cadf>(t0 / moveDuration));
            portion.to = move.from.lerp(move.to, static_cast<cadm::cadf>(t1 / moveDuration));
            applyMove(portion, m_moveIndex);

            m_elapsed += stepAdvancementDuration;
            m_moveElapsed = t1;
            remaining -= stepAdvancementDuration;
        }

        if (m_moveElapsed >= moveDuration - gc_epsilon) {
            ++m_moveIndex;
            m_moveElapsed = 0.0;
        }
    }

    if (m_moveIndex >= m_moves.size()) {
        m_moveIndex = m_moves.size();
        m_moveElapsed = 0.0;
        m_elapsed = m_totalDuration;
        m_fixedStepRemainder = 0.0;
        m_finished = true;
        m_playing = false;
        m_toolPosition = m_moves.back().to;
    }
}

void Simulator::advanceFixed(
    const double simulatedDeltaSeconds,
    const double fixedStepSeconds
    ) {
    if (simulatedDeltaSeconds <= 0.0 || m_moves.empty() || m_finished) {
        return;
    }

    if (fixedStepSeconds <= 0.0) {
        advance(simulatedDeltaSeconds);
        return;
    }

    m_fixedStepRemainder += simulatedDeltaSeconds;
    while (m_fixedStepRemainder + gc_epsilon >= fixedStepSeconds) {
        advance(fixedStepSeconds);
        m_fixedStepRemainder -= fixedStepSeconds;
        if (m_fixedStepRemainder < 0.0) {
            m_fixedStepRemainder = 0.0;
        }
        if (m_finished) {
            m_fixedStepRemainder = 0.0;
            break;
        }
    }
}

void Simulator::finishNow() {
    if (m_moves.empty()) {
        m_finished = true;
        m_playing = false;
        m_fixedStepRemainder = 0.0;
        return;
    }

    if (m_moveIndex < m_moves.size()) {
        const Move& move = m_moves[m_moveIndex];
        if (const double duration = move.durationSeconds();
            m_moveElapsed > 0.0 &&
            duration > 0.0) {
            Move portion = move;
            portion.from = move.from.lerp(
                move.to,
                static_cast<cadm::cadf>(m_moveElapsed / duration)
                );
            applyMove(portion, m_moveIndex);
        } else {
            applyMove(move, m_moveIndex);
        }
        ++m_moveIndex;
    }

    while (m_moveIndex < m_moves.size()) {
        applyMove(m_moves[m_moveIndex], m_moveIndex);
        ++m_moveIndex;
    }

    compactMoves(m_pendingCutterMoves);
    m_moveIndex = m_moves.size();
    m_moveElapsed = 0.0;
    m_elapsed = m_totalDuration;
    m_fixedStepRemainder = 0.0;
    m_finished = true;
    m_playing = false;
    m_toolPosition = m_moves.back().to;
}

ToolPath Simulator::takeCutterMoves() {
    ToolPath result = std::move(m_pendingCutterMoves);
    m_pendingCutterMoves.clear();
    return result;
}

std::size_t Simulator::pendingCutterMoveCount() const {
    return m_pendingCutterMoves.size();
}

void Simulator::applyMove(const Move& move, const std::size_t moveIndex) {
    checkWarnings(move, moveIndex);
    m_pendingCutterMoves.push_back(move);
    m_pendingCutterMoves.back().instructionIndex = moveIndex;
    m_pendingCutterMoves.back().timeline = m_timeline;
    m_toolPosition = move.to;
}

void Simulator::checkWarnings(const Move& move, const std::size_t moveIndex) {
    const float midX = (move.from.x + move.to.x) * 0.5f;
    const float midY = (move.from.y + move.to.y) * 0.5f;
    const bool overStock = containsXY(m_material, midX, midY);
    const float bottom = m_material.topZ - m_material.thickness;
    const float lowestZ = std::min(move.from.z, move.to.z);

    const auto checkIfTooDeep = [&] {
        if (lowestZ < bottom) {
            if (const cadm::Vec3 lowestPoint =
                    move.from.z <= move.to.z
                        ? move.from
                        : move.to;
                containsXY(m_material, lowestPoint.x, lowestPoint.y)) {
                addWarning(
                    WarningKind::tooDeep,
                    moveIndex,
                    move.programIndex,
                    lowestPoint,
                    "Tool tip reaches z=" + formatNumber(lowestZ)
                    + " mm, below the stock base z="
                    + formatNumber(bottom) + " mm."
                    );
            }
        }
    };
    const auto checkIfVerticalPlunge = [&] {
        if (overStock &&
            move.from.xy() == move.to.xy() &&
            move.to.z < move.from.z &&
            lowestZ < m_material.topZ) {
            addWarning(
                WarningKind::verticalPlunge,
                moveIndex,
                move.programIndex,
                move.to,
                "Vertical plunge into material by "
                + formatNumber(move.from.z - move.to.z)
                + " mm."
                );
        }
    };
    checkIfTooDeep();
    checkIfVerticalPlunge();
}

void Simulator::reportShankCollision(Warning warning) {
    if (warning.kind == WarningKind::nonCuttingPart &&
        warning.timeline == m_timeline &&
        warning.moveIndex < m_moves.size()) {
        addWarning(
            warning.kind,
            warning.moveIndex,
            warning.programIndex,
            warning.position,
            std::move(warning.message)
            );
    }
}

void Simulator::addWarning(
    const WarningKind kind,
    const std::size_t moveIndex,
    const std::size_t programIndex,
    const cadm::Vec3 position,
    std::string message
    ) {
    if (const auto key = std::make_pair(kind, moveIndex);
        !m_warningKeys.insert(key).second) {
        return;
    }
    if (m_warnings.size() >= gc_maxWarnings) {
        ++m_suppressedWarnings;
        return;
    }
    m_warnings.push_back({
        .kind = kind,
        .moveIndex = moveIndex,
        .programIndex = programIndex,
        .position = position,
        .message = std::move(message),
        .timeline = m_timeline,
    });
}

Tool Simulator::currentTool() const {
    if (m_moves.empty()) {
        return {};
    }
    if (m_moveIndex < m_moves.size()) {
        return m_moves[m_moveIndex].tool;
    }
    return m_moves.back().tool;
}

Snapshot Simulator::makeSnapshot() {
    if (!m_pathSnapshot) {
        rebuildPathSnapshot();
    }

    Snapshot snapshot{
        .material = m_material,
        .toolPosition = m_toolPosition,
        .tool = currentTool(),
        .playing = m_playing,
        .finished = m_finished,
        .hasPrograms = !m_moves.empty(),
        .elapsed = m_elapsed,
        .totalDuration = m_totalDuration,
        .progress = m_totalDuration > 0.0
                        ? static_cast<float>(std::clamp(m_elapsed / m_totalDuration, 0.0, 1.0))
                        : m_finished && !m_moves.empty()
                        ? 1.0f
                        : 0.0f,
        .instructionCounter = m_moveIndex,
        .instructionCount = m_moves.size(),
        .warnings = m_warnings,
        .suppressedWarnings = m_suppressedWarnings,
        .revision = ++m_revision,
        .moves = m_pathSnapshot,
    };
    return snapshot;
}

const Material& Simulator::material() const {
    return m_material;
}

const ToolPath& Simulator::moves() const {
    return m_moves;
}

const std::vector<Program>& Simulator::programs() const {
    return m_programs;
}

double Simulator::totalDuration() const {
    return m_totalDuration;
}

double Simulator::elapsed() const {
    return m_elapsed;
}

std::size_t Simulator::warningCount() const {
    return m_warnings.size();
}
}
