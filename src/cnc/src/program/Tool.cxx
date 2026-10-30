/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#include "cnc/program/Tool.hxx"

namespace cnc {

float Tool::cuttingLength() const {
    static constexpr float expectedMultiplier{3.0f};
    switch (type) {
    case ToolType::ball:
        return std::max(expectedMultiplier * radius(), 0.1f);
    case ToolType::flat:
        return std::max(expectedMultiplier * diameter, 10.0f);
    }
    return 0.0f;
}

} // namespace cnc
