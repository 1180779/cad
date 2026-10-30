/*
 * Created by Radosław Głasek on 30.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_INCLUDE_CNC_COMPACT_MOVES_HXX
#define CAD_HEADER_SRC_CNC_INCLUDE_CNC_COMPACT_MOVES_HXX

#include "program/Move.hxx"

namespace cnc {
inline bool canMergeMoves(const Move& previous, const Move& move) {
    return previous.timeline == move.timeline &&
        previous.programIndex == move.programIndex &&
        previous.instructionIndex == move.instructionIndex &&
        previous.type == move.type && previous.feed == move.feed &&
        previous.tool.type == move.tool.type &&
        previous.tool.diameter == move.tool.diameter &&
        previous.to == move.from;
}

inline void compactMoves(ToolPath& moves) {
    std::size_t count = 0;
    for (const auto& move : moves) {
        if (count > 0) {
            auto& previous = moves[count - 1];
            if (canMergeMoves(previous, move)) {
                previous.to = move.to;
                continue;
            }
        }
        moves[count++] = move;
    }
    moves.resize(count);
}
}

#endif // CAD_HEADER_SRC_CNC_INCLUDE_CNC_COMPACT_MOVES_HXX
