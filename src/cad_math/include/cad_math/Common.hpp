//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_COMMON_H
#define CAD_COMMON_H

#include <type_traits>

namespace cadm {
    using cadf = float;

    constexpr float gc_feps = 1e-6f;
    constexpr float gc_feps10 = gc_feps * 10;
    constexpr double gc_deps = 1e-12;
    constexpr double gc_deps10 = gc_deps * 10;

    /// @brief Whether <tt>cadf</tt> is single precision
    constexpr bool gc_singlePrecision = std::is_same_v<cadf, float>;

    /// @brief General-purpose epsilon for comparisons and geometric checks
    constexpr cadf gc_eps = gc_singlePrecision
                                ? static_cast<cadf>(gc_feps)
                                : static_cast<cadf>(gc_deps);

    /// @brief Looser epsilon (10x <tt>gc_eps</tt>), for quantities accumulated
    /// over several operations
    constexpr cadf gc_eps10 = gc_singlePrecision
                                  ? static_cast<cadf>(gc_feps10)
                                  : static_cast<cadf>(gc_deps10);
}

#endif //CAD_COMMON_H
