//
// Created by Radosław Głasek on 01.08.2026
//

#ifndef CAD_WRAPDIRECTION_HXX
#define CAD_WRAPDIRECTION_HXX

/// @brief Direction along which a patch grid closes on itself; a cylinder is
/// periodic in exactly one of its parameters
enum class WrapDirection {
    /// @brief Both parameters are bounded to [0, 1]
    none,
    /// @brief u is periodic; the row direction is the seam
    u,
    /// @brief v is periodic; the column direction is the seam
    v,
};

#endif //CAD_WRAPDIRECTION_HXX
