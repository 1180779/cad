//
// Created by Radosław Głasek on 02.07.2026
//

#include "PatchGeometry.hxx"

#include <cmath>
#include <numbers>

#include <cad_math/Mat4.hpp>

#include "Tools.hxx"

using namespace patchgen;

namespace {
    /// @brief Position of grid cell (row, col) for the requested patch surface
    cadm::Vec3 gridPosition(
        const PatchCreateParams &p,
        const int row,
        const int col,
        const int rows,
        const int cols
    ) {
        return std::visit(
            tools::Overloaded{
                [&](const CylinderDimensions &c) -> cadm::Vec3 {
                    const cadm::cadf angle = 2.0f * std::numbers::pi_v<cadm::cadf>
                        * static_cast<cadm::cadf>(col) / static_cast<cadm::cadf>(cols);
                    const cadm::cadf y = -c.height / 2.0f
                        + c.height * static_cast<cadm::cadf>(row) / static_cast<cadm::cadf>(rows - 1);
                    return {c.radius * std::cos(angle), y, c.radius * std::sin(angle)};
                },
                [&](const PlaneExtents &pl) -> cadm::Vec3 {
                    const cadm::cadf x = -pl.width / 2.0f
                        + pl.width * static_cast<cadm::cadf>(col) / static_cast<cadm::cadf>(cols - 1);
                    const cadm::cadf z = -pl.length / 2.0f
                        + pl.length * static_cast<cadm::cadf>(row) / static_cast<cadm::cadf>(rows - 1);
                    return {x, 0.0, z};
                }
            },
            p.dimensions
        );
    }
}

PatchGrid patchgen::generate(const PatchCreateParams &params) {
    const int nx = params.patchCountX;
    const int ny = params.patchCountY;
    const bool wrap = std::visit(
        tools::Overloaded{
            [](const PlaneExtents &) {
                return false;
            },
            [](const CylinderDimensions &) {
                return true;
            }
        },
        params.dimensions
    );

    const bool c2 = params.type == PatchCreateParams::Type::c2;
    const int rows = c2
                         ? ny + 3
                         : 3 * ny + 1;
    const int cols = c2
                         ? (wrap
                                ? nx
                                : nx + 3)
                         : wrap
                         ? 3 * nx
                         : 3 * nx + 1;

    PatchGrid grid;
    grid.rows = rows;
    grid.cols = cols;
    grid.wrapU = wrap;
    grid.patchCountX = nx;
    grid.patchCountY = ny;
    grid.positions.reserve(static_cast<size_t>(rows) * cols);

    const cadm::Mat3 rot = cadm::Mat4::rotZyx(params.orientation).upperLeft3X3();
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid.positions.push_back(params.origin + rot * gridPosition(params, r, c, rows, cols));
        }
    }
    return grid;
}
