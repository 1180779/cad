//
// Created on 5/4/26.
//

#ifndef CAD_VIEWPORTTYPES_HPP
#define CAD_VIEWPORTTYPES_HPP

enum class PivotMode { medianPoint, activeCursor };

enum class TransformMode { none, rotate, scale, translate };

enum class CoordSpace { world, local };

enum class AxisConstraint { none, x, y, z };

#endif //CAD_VIEWPORTTYPES_HPP
