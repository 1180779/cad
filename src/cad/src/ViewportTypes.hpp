//
// Created on 5/4/26.
//

#ifndef CAD_VIEWPORTTYPES_HPP
#define CAD_VIEWPORTTYPES_HPP

#include <iterator>

#include "Macros.hxx"

#define PLANE_ELEMENTS(X) X(xy) X(xz) X(yz)
DECLARE_ENUM_WITH_TO_STRING(Plane, PLANE_ELEMENTS)

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

#define PERFORMANCE_LEVEL_ELEMENTS(X) X(potato) X(low) X(medium) X(high) X(ultra)
DECLARE_ENUM_WITH_TO_STRING(PerformanceLevel, PERFORMANCE_LEVEL_ELEMENTS)

/// @brief Parameters derived from a <tt>PerformanceLevel</tt>, consumed by the
/// renderer to control tessellation detail.
struct PerformanceConfig {
    /// @brief Denominator for adaptive tessellation. Larger = fewer
    /// subdivisions
    int uSubBase = 64;

    float maxTessLevel = 64.0f;

    float minTessLevel = 4.0f;

    /// @brief Return the <tt>PerformanceConfig</tt> for the given
    /// <tt>PerformanceLevel</tt>
    static PerformanceConfig forLevel(const PerformanceLevel level) {
        switch (level) {
        case PerformanceLevel::potato:
            return {
                .uSubBase = 1024,
                .maxTessLevel = 2.0f,
                .minTessLevel = 1.0f
            };
        case PerformanceLevel::low:
            return {
                .uSubBase = 512,
                .maxTessLevel = 8.0f,
                .minTessLevel = 1.0f
            };
        case PerformanceLevel::medium:
            return {
                .uSubBase = 256,
                .maxTessLevel = 16.0f,
                .minTessLevel = 2.0f
            };
        case PerformanceLevel::high:
            return {
                .uSubBase = 128,
                .maxTessLevel = 32.0f,
                .minTessLevel = 4.0f
            };
        case PerformanceLevel::ultra:
            return {
                .uSubBase = 64,
                .maxTessLevel = 64.0f,
                .minTessLevel = 4.0f
            };
        }
        return {
            .uSubBase = 64,
            .maxTessLevel = 64.0f,
            .minTessLevel = 4.0f
        };
    }
};

#endif //CAD_VIEWPORTTYPES_HPP
