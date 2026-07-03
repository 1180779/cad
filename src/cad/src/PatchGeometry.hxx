//
// Created by Radosław Głasek on 02.07.2026
//

#ifndef CAD_PATCHGEOMETRY_HXX
#define CAD_PATCHGEOMETRY_HXX

#include <variant>
#include <vector>

#include <cad_math/Vec3.hpp>

/// @brief Control-point grid generation for joined Bézier patches
namespace patchgen {
    /// @brief Plane extents (columns x rows)
    struct PlaneExtents {
        cadm::cadf width = 5.0;
        cadm::cadf length = 5.0;
    };

    /// @brief Cylinder dimensions
    struct CylinderDimensions {
        cadm::cadf radius = 1.0;
        cadm::cadf height = 5.0;
    };

    /// @brief Parameters for creating a joined Bézier patch
    struct PatchCreateParams {
        /// @brief Patch continuity type
        enum class Type {
            c0,
            c2
        };

        /// @brief Continuity type
        Type type = Type::c0;

        /// @brief Number of single patches around (columns) and along (rows)
        int patchCountX = 1;
        int patchCountY = 1;

        std::variant<PlaneExtents, CylinderDimensions> dimensions;

        /// @brief World-space center the grid is generated around (e.g. the
        /// active 3D cursor), so patches don't always spawn at the origin
        cadm::Vec3 origin{};

        /// @brief Euler ZYX rotation (matching @ref TransformComponent) applied
        /// to the grid around @ref origin, e.g. the active cursor's rotation
        cadm::Vec3 orientation{};
    };

    /// @brief Generated control-point grid: dimensions plus row-major positions
    struct PatchGrid {
        int rows = 0;
        int cols = 0;
        bool wrapU = false;
        int patchCountX = 0;
        int patchCountY = 0;
        std::vector<cadm::Vec3> positions;
    };

    /// @brief Build the control-point grid for the given parameters
    [[nodiscard]] PatchGrid generate(const PatchCreateParams &params);
}

#endif //CAD_PATCHGEOMETRY_HXX
