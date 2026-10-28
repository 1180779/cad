/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#include "cnc/program/ToolType.hxx"

namespace cnc {

std::string_view toolTypeName(const ToolType type) {
    switch (type) {
    case ToolType::ball:
        return "ball";
    case ToolType::flat:
        return "flat";
    }
    return "unknown";
}

} // namespace cnc