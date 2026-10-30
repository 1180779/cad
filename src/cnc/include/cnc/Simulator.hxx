//
// Created by Radosław Głasek on 18.10.2026
//

#ifndef CAD_SIMULATOR_HXX
#define CAD_SIMULATOR_HXX

#include "Snapshot.hxx"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cnc/Material.hxx"
#include "program/NcProgram.hxx"

namespace cnc {
/// @brief Thread-confined toolpath timeline. One instance belongs to the
/// simulation worker; snapshots and cutter moves are the only data handed
/// to the render thread
class Simulator {
public:
    explicit Simulator(const Material& material = {});

    /// @brief Replace the stock description and reset the timeline
    /// @note Loaded programs are kept
    /// @note Resetting the GPU height map is the caller's responsibility
    void setMaterial(const Material& material);

    /// @brief Rewind the timeline
    /// @note Resetting the GPU height map is the caller's responsibility
    void resetMaterial();

    /// @brief Append a parsed program
    /// @note Preserves the material
    void appendProgram(Program program);

    /// @brief Discard all loaded programs. The caller may keep or reset the
    /// GPU height map independently
    /// @note Resetting the GPU height map is left to the caller's judgement
    void clearPrograms();
    void play();
    void pause();
    void setPlaying(bool playing);
    [[nodiscard]] bool playing() const;
    [[nodiscard]] bool finished() const;

    void advance(double simulatedSeconds);

    /// @brief Advance using a fixed internal simulation step
    /// @param simulatedDeltaSeconds Simulated time to consume
    /// @param fixedStepSeconds Step size in simulated seconds; must be > 0
    void advanceFixed(double simulatedDeltaSeconds, double fixedStepSeconds);
    void finishNow();

    /// @brief Cutter moves that still need to be rendered into the GPU
    /// height map
    [[nodiscard]] ToolPath takeCutterMoves();
    [[nodiscard]] std::size_t pendingCutterMoveCount() const;
    void reportStockCollision(Warning warning);
    [[nodiscard]] Snapshot makeSnapshot();
    [[nodiscard]] const Material& material() const;
    [[nodiscard]] const ToolPath& moves() const;
    [[nodiscard]] const std::vector<Program>& programs() const;
    [[nodiscard]] double totalDuration() const;
    [[nodiscard]] double elapsed() const;
    [[nodiscard]] std::size_t warningCount() const;

private:
    void resetTimeline();
    void rebuildPathSnapshot();
    void applyMove(const Move& move, std::size_t moveIndex);
    void checkWarnings(const Move& move, std::size_t moveIndex);

    void addWarning(
        WarningKind kind,
        std::size_t moveIndex,
        std::size_t programIndex,
        cadm::Vec3 position,
        std::string message
        );

    [[nodiscard]] Tool currentTool() const;

    Material m_material;
    ToolPath m_moves;
    std::vector<Program> m_programs;
    ToolPath m_pendingCutterMoves;
    std::shared_ptr<const ToolPath> m_pathSnapshot;

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
    std::uint64_t m_timeline{};

    static constexpr std::size_t gc_maxWarnings = 1024;
    static constexpr double gc_epsilon = 1e-12;
};
}

#endif //CAD_SIMULATOR_HXX
