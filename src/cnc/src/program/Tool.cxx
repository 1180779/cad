/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#include "cnc/program/Tool.hxx"

namespace cnc {
float Tool::cuttingLength() const {
    switch (type) {
    case ToolType::ball:
        return std::max(radius(), 0.1f);
    case ToolType::flat:
        return std::max(3.0f * diameter, 10.0f);
    }
    return 0.0f;
}
} // namespace cnc
