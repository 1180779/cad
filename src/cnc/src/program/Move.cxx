/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#include "cnc/program/Move.hxx"

namespace cnc {

float Move::length() const {
    return (to - from).length();
}

double Move::durationSeconds() const {
    float speed{};
    switch (type) {
    case MoveType::G00:
        speed = gc_rapidFeed;
        break;
    case MoveType::G01:
        speed = feed > 0.0f ? feed : gc_defaultFeed;
        break;
    }
    return static_cast<double>(length()) / static_cast<double>(speed) * 60.0;
}

} // namespace cnc
