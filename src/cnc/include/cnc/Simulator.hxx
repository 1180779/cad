//
// Created by Radosław Głasek on 18.10.2026
//

#ifndef CAD_SIMULATOR_HXX
#define CAD_SIMULATOR_HXX

#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cnc/Material.hxx"
#include "program/NcProgram.hxx"

namespace cnc {
    enum class WarningKind {
        nonCuttingPart,
        tooDeep,
        verticalPlunge,
    };

    [[nodiscard]] std::string_view warningKindName(WarningKind kind);

    struct Warning {
        WarningKind kind{WarningKind::nonCuttingPart};
        std::size_t moveIndex{};
        std::size_t programIndex{};
        cadm::Vec3 position{};
        std::string message;
    };

    /// @brief Immutable state consumed by the renderer/UI thread
    ///
    /// @note The <material> height map is intentionally not part of this
    /// snapshot: it lives on the GPU and is updated by rendering cutter moves
    /// into the <height> texture. The CPU simulator only owns the toolpath
    /// timeline
    struct Snapshot {
        MaterialConfig material{};

        cadm::Vec3 toolPosition{};
        Tool tool{};
        bool playing{};
        bool finished{};
        bool hasPrograms{};

        double elapsed{};
        double totalDuration{};
        float progress{};
        std::size_t instructionCounter{};
        std::size_t instructionCount{};

        std::vector<Warning> warnings;
        std::size_t suppressedWarnings{};

        std::uint64_t revision{};

        /// @brief Shared immutable copy of the path, so the renderer can
        /// rebuild its line buffer without touching the live move list
        std::shared_ptr<const std::vector<Move>> moves;
    };

    /// @brief Thread-confined toolpath timeline. One instance belongs to the
    /// simulation worker; snapshots and cutter moves are the only data handed
    /// to the render thread
    class Simulator {
    public:
        explicit Simulator(const MaterialConfig& material = {});

        /// @brief Replace the stock description and reset the timeline. Loaded
        /// programs are kept so they can be simulated again on the new stock
        /// @note Resetting the GPU height map is the caller's responsibility
        void setMaterial(const MaterialConfig& material);

        /// @brief Rewind the timeline. The caller also resets the GPU texture
        void resetMaterial();

        /// @brief Append a parsed program. Material is preserved across appends,
        /// which is what allows multiple files to be milled in sequence
        void appendProgram(Program program);

        /// @brief Discard all loaded programs. The caller may keep or reset the
        /// GPU height map independently
        void clearPrograms();

        void play();

        void pause();

        void setPlaying(bool playing);

        [[nodiscard]] bool playing() const {
            return m_playing;
        }

        [[nodiscard]] bool finished() const {
            return m_finished;
        }

        /// @brief Advance the simulation by a simulated-time delta in seconds.
        /// Cutting portions are appended to the pending cutter queue
        void advance(double simulatedSeconds);

        /// @brief Advance using a fixed internal simulation step
        ///
        /// The host passes a simulated-time delta (for example, real frame
        /// time multiplied by the speed factor), but cutter moves are always
        /// generated at fixed simulated-time increments. Host frame jitter or
        /// load therefore cannot change the geometry of the cuts.
        /// @param simulatedDeltaSeconds Simulated time to consume
        /// @param fixedStepSeconds Step size in simulated seconds; must be > 0.
        void advanceFixed(double simulatedDeltaSeconds, double fixedStepSeconds);

        /// @brief Advance all the way to the end, appending every remaining
        /// cutting move to the pending cutter queue.
        void finishNow();

        /// @brief Cutter moves that still need to be rendered into the GPU
        /// height map. The caller submits them in fixed-size batches.
        [[nodiscard]] std::vector<Move> takeCutterMoves();

        [[nodiscard]] std::size_t pendingCutterMoveCount() const {
            return m_pendingCutterMoves.size();
        }

        /// @brief Build a consistent copy of the current timeline state
        [[nodiscard]] Snapshot makeSnapshot();

        [[nodiscard]] const MaterialConfig& material() const {
            return m_material;
        }

        [[nodiscard]] const std::vector<Move>& moves() const {
            return m_moves;
        }

        [[nodiscard]] const std::vector<Program>& programs() const {
            return m_programs;
        }

        [[nodiscard]] double totalDuration() const {
            return m_totalDuration;
        }

        [[nodiscard]] double elapsed() const {
            return m_elapsed;
        }

        [[nodiscard]] std::size_t warningCount() const {
            return m_warnings.size();
        }

    private:
        void resetTimeline();

        void rebuildPathSnapshot();

        void applyMove(const Move &move, std::size_t moveIndex);

        void checkWarnings(const Move &move, std::size_t moveIndex);

        void addWarning(
            WarningKind kind,
            std::size_t moveIndex,
            std::size_t programIndex,
            cadm::Vec3 position,
            std::string message
        );

        [[nodiscard]] Tool currentTool() const;

        MaterialConfig m_material;
        std::vector<Move> m_moves;
        std::vector<Program> m_programs;
        std::vector<Move> m_pendingCutterMoves;
        std::shared_ptr<const std::vector<Move>> m_pathSnapshot;

        std::size_t m_moveIndex{};
        double m_moveElapsed{};
        double m_elapsed{};
        double m_totalDuration{};
        double m_fixedStepRemainder{};
        cadm::Vec3 m_toolPosition{};

        bool m_playing{};
        bool m_finished{};

        std::vector<Warning> m_warnings;
        std::set<std::pair<WarningKind, std::size_t>> m_warningKeys;
        std::size_t m_suppressedWarnings{};
        std::uint64_t m_revision{};

        static constexpr std::size_t gc_maxWarnings = 128;
        static constexpr double gc_epsilon = 1e-12;
    };
}

#endif //CAD_SIMULATOR_HXX
