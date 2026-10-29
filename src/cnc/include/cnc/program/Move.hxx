/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_INCLUDE_CNC_MOVE_HXX
#define CAD_HEADER_SRC_CNC_INCLUDE_CNC_MOVE_HXX

#include "MoveType.hxx"
#include "Tool.hxx"
#include "cad_math/Vec3.hxx"

#include <vector>

namespace cnc
{
    /// @brief Rapid move feed; in mm/min
    inline constexpr float gc_rapidFeed = 20000.0f;

    /// @brief Default cutting feed; in mm/min
    inline constexpr float gc_defaultFeed = 15000.0f;

    /// @brief One linear move of the machine
/// @note Only G00/G01 moves are supported
    struct Move
    {
        cadm::Vec3 from{};
        cadm::Vec3 to{};
        /// @brief Move type
        MoveType type{MoveType::G01};
        /// @brief Feed rate in mm/min
        float feed{gc_defaultFeed};
        Tool tool{};
        /// @brief Index of the source program instruction
        std::size_t programIndex{0};

        [[nodiscard]] float length() const;
        [[nodiscard]] double durationSeconds() const;
    };

    using ToolPath = std::vector<Move>;
} // namespace cnc

#endif // CAD_HEADER_SRC_CNC_INCLUDE_CNC_MOVE_HXX
