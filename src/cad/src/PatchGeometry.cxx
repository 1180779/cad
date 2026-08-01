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
        const bool c2 = p.type == PatchCreateParams::Type::c2;
        const auto c0fraction = [](const int i, const int n) {
            return static_cast<cadm::cadf>(i) / static_cast<cadm::cadf>(n - 1);
        };
        const auto c2fraction = [](const int i, const int n) {
            return static_cast<cadm::cadf>(i - 1) / static_cast<cadm::cadf>(n - 1 - 2);
        };
        const auto fractionF = c2
                                   ? c2fraction
                                   : c0fraction;

        const auto c0radius = [](const CylinderDimensions &c, const int) {
            return c.radius;
        };
        // at t_i: d_{i - 1} + 4d_{i} + d_{i + 1}
        // 
        // split by radial and tangent:
        // radial:  R'cos(φ) + 4R' + R'cos(φ) / 6
        // tangent: -R'sin(φ) + 0 + R'sin(φ) = 0
        // 
        // so: R = 2/3R' + 1/3R'cos(φ) = R'(2+cos(φ))/3
        //  => R' = R * 3 / (2+cos(φ))
        const auto c2radius = [](const CylinderDimensions &c, const int lCols) {
            return c.radius * 3.0f / (
                2.0f + std::cos(2.0f * std::numbers::pi_v<cadm::cadf> / static_cast<cadm::cadf>(lCols))
            );
        };
        const auto radiusF = c2
                                 ? c2radius
                                 : c0radius;

        return std::visit(
            tools::Overloaded{
                [&](const CylinderDimensions &c) -> cadm::Vec3 {
                    const bool aroundRows = c.seam == WrapDirection::u;
                    const int aroundIdx = aroundRows
                                              ? row
                                              : col;
                    const int aroundCount = aroundRows
                                                ? rows
                                                : cols;
                    const cadm::cadf angle = 2.0f * std::numbers::pi_v<cadm::cadf>
                        * static_cast<cadm::cadf>(aroundIdx) / static_cast<cadm::cadf>(aroundCount);
                    const cadm::cadf radius = radiusF(c, aroundCount);
                    const cadm::cadf y = -c.height * 0.5f + c.height * (
                        aroundRows
                            ? fractionF(col, cols)
                            : fractionF(row, rows)
                    );
                    return {radius * std::cos(angle), y, radius * std::sin(angle)};
                },
                [&](const PlaneExtents &pl) -> cadm::Vec3 {
                    const cadm::cadf x = -pl.width * 0.5f + pl.width * fractionF(col, cols);
                    const cadm::cadf z = -pl.length * 0.5f + pl.length * fractionF(row, rows);
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
    const WrapDirection wrap = std::visit(
        tools::Overloaded{
            [](const PlaneExtents &) {
                return WrapDirection::none;
            },
            [](const CylinderDimensions &c) {
                return c.seam;
            }
        },
        params.dimensions
    );

    const bool c2 = params.type == PatchCreateParams::Type::c2;
    const auto extent = [c2](const int count, const bool periodic) {
        const int base = c2
                             ? count
                             : 3 * count;
        return periodic
                   ? base
                   : base + (c2
                                 ? 3
                                 : 1);
    };
    const int rows = extent(ny, wrap == WrapDirection::u);
    const int cols = extent(nx, wrap == WrapDirection::v);

    PatchGrid grid;
    grid.rows = rows;
    grid.cols = cols;
    grid.wrap = wrap;
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
