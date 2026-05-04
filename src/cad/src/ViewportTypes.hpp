//
// Created on 5/4/26.
//

#ifndef CAD_VIEWPORTTYPES_HPP
#define CAD_VIEWPORTTYPES_HPP

enum class PivotMode { MedianPoint, ActiveCursor };

enum class TransformMode { None, Rotate, Scale, Translate };

enum class CoordSpace { World, Local };

enum class AxisConstraint { None, X, Y, Z };

#endif //CAD_VIEWPORTTYPES_HPP
