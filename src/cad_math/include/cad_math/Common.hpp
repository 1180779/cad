//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_COMMON_H
#define CAD_COMMON_H

namespace cadm {
    using cadf = float;

    constexpr float gc_feps = 1e-6f;
    constexpr double gc_deps = 1e-12;

    /// @brief General-purpose epsilon for float comparisons and geometric checks
    constexpr cadf gc_eps = gc_feps;
}

#endif //CAD_COMMON_H
