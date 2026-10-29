/*
 * Created by Radosław Głasek on 29.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_INCLUDE_CNC_SNAPSHOT_HXX
#define CAD_HEADER_SRC_CNC_INCLUDE_CNC_SNAPSHOT_HXX

#include "Material.hxx"

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

/// @brief Immutable state consumed by another thread
///
/// @note The <material> height map is not part of the snapshot: it lives on
/// the GPU and is updated by rendering cutter moves into the <height>
/// texture. The CPU simulator only owns the toolpath timeline
struct Snapshot {
    Material material{};

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

    /// @brief Shared immutable copy of the path, so renderer can rebuild line
    /// buffer without touching the move list
    std::shared_ptr<const ToolPath> moves;
};

} // namespace cnc

#endif // CAD_HEADER_SRC_CNC_INCLUDE_CNC_SNAPSHOT_HXX
