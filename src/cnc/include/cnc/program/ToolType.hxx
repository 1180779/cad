/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_INCLUDE_CNC_TOOL_TYPE_HXX
#define CAD_HEADER_SRC_CNC_INCLUDE_CNC_TOOL_TYPE_HXX
#include <string_view>

namespace cnc {

/// @brief Geometry of the cutting tool
enum class ToolType {
    ball,
    flat,
};

    std::string_view toolTypeName(ToolType type);

} // namespace cnc


#endif // CAD_HEADER_SRC_CNC_INCLUDE_CNC_TOOL_TYPE_HXX