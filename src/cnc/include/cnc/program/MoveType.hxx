/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_INCLUDE_CNC_MOVE_TYPE_HXX
#define CAD_HEADER_SRC_CNC_INCLUDE_CNC_MOVE_TYPE_HXX

namespace cnc
{
    enum class MoveType
    {
        /* rapid move */
        G00,
        /* cutting feed */
        G01
    };
} // namespace cnc

#endif // CAD_HEADER_SRC_CNC_INCLUDE_CNC_MOVE_TYPE_HXX
