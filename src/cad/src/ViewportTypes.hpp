//
// Created on 5/4/26.
//

#ifndef CAD_VIEWPORTTYPES_HPP
#define CAD_VIEWPORTTYPES_HPP

enum class PivotMode { medianPoint, activeCursor };

enum class TransformMode { none, rotate, scale, translate };

enum class CoordSpace { world, local };

enum class AxisConstraint {
    none, x, y, z
};

namespace axisConstraint {
    inline int fromEnum(const AxisConstraint c) {
        switch (c) {
        case AxisConstraint::x:
            return 1;
        case AxisConstraint::y:
            return 2;
        case AxisConstraint::z:
            return 4;
        default: ;
        }
        return 0;
    }
}

#endif //CAD_VIEWPORTTYPES_HPP
