/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_INCLUDE_CNC_TOOL_HXX
#define CAD_HEADER_SRC_CNC_INCLUDE_CNC_TOOL_HXX

#include "ToolType.hxx"

namespace cnc {
enum class ToolType;

/// @brief Tool description (diameter in millimetres)
struct Tool {
    ToolType type{ToolType::ball};
    /// @brief Cutting diameter in mm
    float diameter{6.0f};

    [[nodiscard]] float radius() const {
        return diameter * 0.5f;
    }

    /// @brief Height of the cutting portion above the tool tip, in mm
    ///
    /// Material touching the tool above this height is hit by the
    /// non-cutting shank. This is an advisory collision model: a ball mill
    /// uses the hemisphere (one diameter), a flat mill a conservative flute
    /// length
    [[nodiscard]] float cuttingLength() const;
};

} // namespace cnc


#endif // CAD_HEADER_SRC_CNC_INCLUDE_CNC_TOOL_HXX