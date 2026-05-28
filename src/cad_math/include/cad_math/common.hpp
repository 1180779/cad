//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_COMMON_H
#define CAD_COMMON_H

namespace cadm {
    using cadf = float;

    constexpr float feps = 1e-6f;
    constexpr double deps = 1e-12;

    // General-purpose epsilon for float comparisons and geometric checks.
    constexpr cadf eps = feps;
}

#endif //CAD_COMMON_H
