//
// Created by Radosław Głasek on 18.10.2026
//

#include "cnc/Simulator.hxx"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <sstream>

namespace cnc
{
    namespace
    {
        std::string formatNumber(const double value)
        {
            std::ostringstream stream;
            stream << std::fixed << std::setprecision(2) << value;
            return stream.str();
        }

        bool containsXY(const MaterialConfig& material, const float x, const float y)
        {
            const float halfWidth = material.width * 0.5f;
            const float halfDepth = material.depth * 0.5f;
            return x >= material.centerX - halfWidth
                && x <= material.centerX + halfWidth
                && y >= material.centerY - halfDepth
                && y <= material.centerY + halfDepth;
        }

        bool isOutsideStock(const MaterialConfig& material, const cadm::Vec3& point)
        {
            if (!containsXY(material, point.x, point.y))
            {
                return true;
            }
            return point.z >= material.topZ - 1e-3f;
        }
    }

    std::string_view warningKindName(const WarningKind kind)
    {
        switch (kind)
        {
        case WarningKind::nonCuttingPart:
            return "non-cutting part";
        case WarningKind::tooDeep:
            return "too deep";
        case WarningKind::verticalPlunge:
            return "vertical plunge";
        }
        return "unknown";
    }

    Simulator::Simulator(const MaterialConfig& material)
        : m_material(material.normalized())
    {
        rebuildPathSnapshot();
        resetTimeline();
    }

    void Simulator::setMaterial(const MaterialConfig& material)
    {
        m_material = material.normalized();
        if (!m_moves.empty())
        {
            if (Move& first = m_moves.front();
                first.type == MoveType::G00
                && containsXY(m_material, first.to.x, first.to.y)
                && first.from.z < m_material.topZ)
            {
                first.from.z = m_material.topZ
                    + std::max(5.0f, first.tool.diameter);
            }
        }
        rebuildPathSnapshot();
        resetTimeline();
    }

    void Simulator::resetMaterial()
    {
        resetTimeline();
    }

    void Simulator::appendProgram(Program program)
    {
        if (program.moves.empty())
        {
            m_programs.push_back(std::move(program));
            return;
        }

        const bool wasEmpty = m_moves.empty();
        const float clearance = std::max(5.0f, program.tool.diameter);
        const Move& first = program.moves.front();

        if (isOutsideStock(m_material, first.to))
        {
            const cadm::Vec3 safeTo = first.to;
            cadm::Vec3 approachFrom = safeTo;
            if (containsXY(m_material, safeTo.x, safeTo.y))
            {
                approachFrom.z = std::max(safeTo.z, m_material.topZ + clearance);
            }

            Move approach{
                .from = wasEmpty ? approachFrom : m_toolPosition,
                .to = safeTo,
                .type = MoveType::G00,
                .feed = gc_rapidFeed,
                .tool = program.tool,
            };

            if (wasEmpty)
            {
                program.moves.front() = approach;
            }
            else
            {
                program.moves.erase(program.moves.begin());
                program.moves.insert(program.moves.begin(), approach);
            }
        }
        else if (wasEmpty
            && containsXY(m_material, first.from.x, first.from.y)
            && first.from.z >= m_material.topZ - 1e-3f)
        {
            // a program that starts directly with a plunge still gets an
            // explicit approach from above the stock
            cadm::Vec3 approachFrom = first.from;
            approachFrom.z = std::max(first.from.z, m_material.topZ + clearance);
            if (approachFrom.z > first.from.z + 1e-3f)
            {
                program.moves.insert(
                    program.moves.begin(),
                    Move{
                        .from = approachFrom,
                        .to = first.from,
                        .type = MoveType::G00,
                        .feed = gc_rapidFeed,
                        .tool = program.tool,
                    }
                );
            }
        }

        const std::size_t programIndex = m_programs.size();
        for (Move& move : program.moves)
        {
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
        if (wasEmpty)
        {
            resetTimeline();
        }
    }

    void Simulator::clearPrograms()
    {
        m_moves.clear();
        m_programs.clear();
        rebuildPathSnapshot();
        resetTimeline();
    }

    void Simulator::play()
    {
        if (!m_moves.empty() && !m_finished)
        {
            m_playing = true;
        }
    }

    void Simulator::pause()
    {
        m_playing = false;
    }

    void Simulator::setPlaying(const bool playing)
    {
        if (playing)
        {
            play();
        }
        else
        {
            pause();
        }
    }

    void Simulator::resetTimeline()
    {
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

    void Simulator::rebuildPathSnapshot()
    {
        m_pathSnapshot = std::make_shared<const std::vector<Move>>(m_moves);
        m_totalDuration = 0.0;
        for (const Move& move : m_moves)
        {
            m_totalDuration += move.durationSeconds();
        }
    }

    void Simulator::advance(const double simulatedSeconds)
    {
        if (simulatedSeconds <= 0.0 || m_moves.empty() || m_finished)
        {
            return;
        }

        double remaining = simulatedSeconds;
        while (m_moveIndex < m_moves.size())
        {
            const Move& move = m_moves[m_moveIndex];
            const double moveDuration = move.durationSeconds();

            if (moveDuration <= epsilon)
            {
                applyMove(move, m_moveIndex);
                ++m_moveIndex;
                m_moveElapsed = 0.0;
                continue;
            }

            if (remaining <= 0.0)
            {
                break;
            }

            const double remainingMoveDuration = moveDuration - m_moveElapsed;
            const double stepAdvancementDuration = std::min(remaining, remainingMoveDuration);
            const double t0 = m_moveElapsed;
            const double t1 = t0 + stepAdvancementDuration;

            if (stepAdvancementDuration > 0.0)
            {
                Move portion = move;
                portion.from = move.from.lerp(move.to, static_cast<cadm::cadf>(t0 / moveDuration));
                portion.to = move.from.lerp(move.to, static_cast<cadm::cadf>(t1 / moveDuration));
                applyMove(portion, m_moveIndex);

                m_elapsed += stepAdvancementDuration;
                m_moveElapsed = t1;
                remaining -= stepAdvancementDuration;
            }

            if (m_moveElapsed >= moveDuration - epsilon)
            {
                ++m_moveIndex;
                m_moveElapsed = 0.0;
            }
        }

        if (m_moveIndex >= m_moves.size())
        {
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
    )
    {
        if (simulatedDeltaSeconds <= 0.0 || m_moves.empty() || m_finished)
        {
            return;
        }

        if (fixedStepSeconds <= 0.0)
        {
            advance(simulatedDeltaSeconds);
            return;
        }

        m_fixedStepRemainder += simulatedDeltaSeconds;
        while (m_fixedStepRemainder + epsilon >= fixedStepSeconds)
        {
            advance(fixedStepSeconds);
            m_fixedStepRemainder -= fixedStepSeconds;
            if (m_fixedStepRemainder < 0.0)
            {
                m_fixedStepRemainder = 0.0;
            }
            if (m_finished)
            {
                m_fixedStepRemainder = 0.0;
                break;
            }
        }
    }

    void Simulator::finishNow()
    {
        if (m_moves.empty())
        {
            m_finished = true;
            m_playing = false;
            m_fixedStepRemainder = 0.0;
            return;
        }

        if (m_moveIndex < m_moves.size())
        {
            const Move& move = m_moves[m_moveIndex];
            if (const double duration = move.durationSeconds();
                m_moveElapsed > 0.0 && duration > 0.0)
            {
                Move portion = move;
                portion.from = move.from.lerp(move.to, static_cast<cadm::cadf>(m_moveElapsed / duration));
                applyMove(portion, m_moveIndex);
            }
            else
            {
                applyMove(move, m_moveIndex);
            }
            ++m_moveIndex;
        }

        while (m_moveIndex < m_moves.size())
        {
            applyMove(m_moves[m_moveIndex], m_moveIndex);
            ++m_moveIndex;
        }

        m_moveIndex = m_moves.size();
        m_moveElapsed = 0.0;
        m_elapsed = m_totalDuration;
        m_fixedStepRemainder = 0.0;
        m_finished = true;
        m_playing = false;
        m_toolPosition = m_moves.back().to;
    }

    std::vector<Move> Simulator::takeCutterMoves()
    {
        std::vector<Move> result = std::move(m_pendingCutterMoves);
        m_pendingCutterMoves.clear();
        return result;
    }

    void Simulator::applyMove(const Move& move, const std::size_t moveIndex)
    {
        checkWarnings(move, moveIndex);
        if (move.type != MoveType::G00)
        {
            m_pendingCutterMoves.push_back(move);
        }
        m_toolPosition = move.to;
    }

    void Simulator::checkWarnings(const Move& move, const std::size_t moveIndex)
    {
        const Tool& tool = move.tool;
        const float midX = (move.from.x + move.to.x) * 0.5f;
        const float midY = (move.from.y + move.to.y) * 0.5f;
        const bool overStock = containsXY(m_material, midX, midY);
        const float bottom = m_material.topZ - m_material.thickness;
        const float lowestZ = std::min(move.from.z, move.to.z);

        const auto checkIfTooDeep = [&]
        {
            if (lowestZ < bottom - 1e-4f)
            {
                const cadm::Vec3 lowestPoint = move.from.z <= move.to.z ? move.from : move.to;
                if (containsXY(m_material, lowestPoint.x, lowestPoint.y))
                {
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
        const auto checkIfVerticalPlunge = [&]
        {
            const float dx = move.to.x - move.from.x;
            const float dy = move.to.y - move.from.y;
            if (const float horizontalLength = std::sqrt(dx * dx + dy * dy);
                overStock
                && horizontalLength < 0.05f
                && move.to.z < move.from.z - 1e-4f
                && lowestZ < m_material.topZ)
            {
                addWarning(
                    WarningKind::verticalPlunge,
                    moveIndex,
                    move.programIndex,
                    move.to,
                    "Vertical plunge into material by "
                    + formatNumber(move.from.z - move.to.z) + " mm."
                );
            }
        };
        const auto checkIfNonCuttingPart = [&]
        {
            // Conservative first version: the original top surface is used
            // instead of the current GPU height map. This may report a shank
            // collision where material was already removed, but it never misses
            // one. The GPU pass can supply exact flags later.
            const float top = m_material.topZ;
            const float cuttingLength = tool.cuttingLength();
            for (const double t : {0.0, 0.5, 1.0})
            {
                const cadm::Vec3 point = move.from.lerp(move.to, static_cast<cadm::cadf>(t));
                if (!containsXY(m_material, point.x, point.y))
                {
                    continue;
                }
                if (top > point.z + cuttingLength + 1e-4f)
                {
                    addWarning(
                        WarningKind::nonCuttingPart,
                        moveIndex,
                        move.programIndex,
                        point,
                        "Stock top z=" + formatNumber(top) + " is above the cutting length "
                        + formatNumber(cuttingLength)
                        + " mm of a " + formatNumber(tool.diameter)
                        + " mm " + std::string(toolTypeName(tool.type))
                        + " tool; the non-cutting shank may rub."
                    );
                    break;
                }
            }
        };

        checkIfTooDeep();
        checkIfVerticalPlunge();
        checkIfNonCuttingPart();
    }

    void Simulator::addWarning(
        const WarningKind kind,
        const std::size_t moveIndex,
        const std::size_t programIndex,
        const cadm::Vec3 position,
        std::string message
    )
    {
        if (const auto key = std::make_pair(kind, moveIndex);
            !m_warningKeys.insert(key).second)
        {
            return;
        }
        if (m_warnings.size() >= gc_maxWarnings)
        {
            ++m_suppressedWarnings;
            return;
        }
        m_warnings.push_back({
            kind,
            moveIndex,
            programIndex,
            position,
            std::move(message),
        });
    }

    Tool Simulator::currentTool() const
    {
        if (m_moves.empty())
        {
            return {};
        }
        if (m_moveIndex < m_moves.size())
        {
            return m_moves[m_moveIndex].tool;
        }
        return m_moves.back().tool;
    }

    Snapshot Simulator::makeSnapshot()
    {
        if (!m_pathSnapshot)
        {
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
}
