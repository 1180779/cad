//
// Created by Radosław Głasek on 09.07.2026
//

#ifndef CAD_POINTHANDLE_HXX
#define CAD_POINTHANDLE_HXX

#include <cstdint>
#include <limits>

/// @brief Stable index into PointRegistry's slot array. Remains valid even
/// after other points are removed. Bézier curves and surfaces store these to
/// reference shared control points
using PointHandle = uint32_t;

static constexpr PointHandle InvalidPointHandle = std::numeric_limits<uint32_t>::max();

#endif //CAD_POINTHANDLE_HXX
