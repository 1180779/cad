//
// Created by Radosław Głasek on 02.08.2026
//

#ifndef CAD_TRIMMASK_HXX
#define CAD_TRIMMASK_HXX

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <cad_math/Vec2.hpp>

/// @brief Splitting a surface's parameter square along an intersection curve
namespace trimming {
    /// @brief Which of a surface's parameters are periodic
    struct SurfaceWrap {
        bool u = false,
             v = false;
    };

    enum class Cell : std::uint8_t {
        /// @brief Region reachable from the flood-fill seed without crossing
        /// the curve
        inside = 0,
        /// @brief The other side
        outside = 1,
        /// @brief Crossed by the curve itself
        border = 2,
    };

    /// @brief Whether @p cell belongs to the kept region
    /// @param cell Checked cell
    /// @param keepInside Which of the two regions counts as kept; the border
    /// always counts as kept, so the curve itself stays drawn
    [[nodiscard]] constexpr bool kept(const Cell cell, const bool keepInside) {
        return cell == Cell::border || (cell == Cell::inside) == keepInside;
    }

    /// @brief Wrap @p t into [0, 1) and map it onto a grid cell index
    [[nodiscard]] inline int toGrid(const cadm::cadf t, const int size) {
        const auto wrapped = t - std::floor(t);
        return std::clamp(static_cast<int>(wrapped * static_cast<cadm::cadf>(size)), 0, size - 1);
    }

    /// @brief Parameter square rasterized into a square grid, split in two by
    /// an intersection curve
    struct TrimMask {
        int size = 0;
        std::vector<Cell> cells;

        [[nodiscard]] Cell at(const int x, const int y) const {
            return cells.at(static_cast<std::size_t>(y) * size + x);
        }

        [[nodiscard]] Cell operator[](const int x, const int y) const {
            return cells[static_cast<std::size_t>(y) * size + x];
        }

        Cell& operator[](const int x, const int y) {
            return cells[static_cast<std::size_t>(y) * size + x];
        }

        [[nodiscard]] bool empty() const {
            return size == 0;
        }

        /// @brief Whether (@p u, @p v) falls in the region being kept
        /// @param u,v parameter point to classify
        /// @param keepInside Which of the two regions counts as kept; the
        /// border always counts as kept, so the curve itself stays drawn
        [[nodiscard]] bool keeps(const cadm::cadf u, const cadm::cadf v, const bool keepInside) const {
            if (empty()) {
                return true;
            }
            // TODO: measure performance & change to unchecked access if this is
            //  slow
            return kept(at(toGrid(u, size), toGrid(v, size)), keepInside);
        }
    };

    /// @brief Mask kept only where both inputs keep, for trimming one surface
    /// along both parameter curves of a self-intersection
    [[nodiscard]] inline TrimMask combineKept(
        const TrimMask &a,
        const bool keepA,
        const TrimMask &b,
        const bool keepB
    ) {
        TrimMask out{
            .size = a.size,
            .cells = std::vector<Cell>(a.cells.size())
        };
        for (std::size_t i = 0; i < a.cells.size(); ++i) {
            if (a.cells[i] == Cell::border || b.cells[i] == Cell::border) {
                out.cells[i] = Cell::border;
            }
            else {
                out.cells[i] = kept(a.cells[i], keepA) && kept(b.cells[i], keepB)
                                   ? Cell::inside
                                   : Cell::outside;
            }
        }
        return out;
    }

    /// @brief A surface's trimming state: which half of its parameter square is
    /// hidden, if any
    struct TrimState {
        TrimMask mask;
        bool keepInside = true;
        bool enabled = false;

        /// @brief Whether (@p u, @p v) is drawn
        [[nodiscard]] bool keeps(const cadm::cadf u, const cadm::cadf v) const {
            return !enabled || mask.keeps(u, v, keepInside);
        }

        void set(TrimMask m, const bool keep) {
            mask = std::move(m);
            keepInside = keep;
            enabled = !mask.empty();
        }

        void clear() {
            mask = {};
            enabled = false;
        }
    };

    /// @brief Mark every cell the segment @p a -> @p b passes through as border
    inline void strokeSegment(
        TrimMask &mask,
        const cadm::Vec2 &a,
        const cadm::Vec2 &b,
        const SurfaceWrap wrap
    ) {
        auto delta = b - a;
        if (wrap.u) {
            delta.x -= std::round(delta.x);
        }
        if (wrap.v) {
            delta.y -= std::round(delta.y);
        }
        const auto steps = std::max(
            1,
            static_cast<int>(
                std::ceil(
                    std::max(
                        std::abs(delta.x),
                        std::abs(delta.y)
                    )
                    * static_cast<cadm::cadf>(mask.size) * 2
                )
            )
        );
        for (int i = 0; i <= steps; ++i) {
            const auto t = static_cast<cadm::cadf>(i) / static_cast<cadm::cadf>(steps);
            const auto p = a + delta * t;
            mask[toGrid(p.x, mask.size), toGrid(p.y, mask.size)] = Cell::border;
        }
    }

    /// @brief Stroke from @p end to the nearest non-periodic domain edge
    /// @note The march stops just short of the domain edge, and a border that
    /// does not reach it would let the fill leak around the curve's ends
    inline void extendToEdge(TrimMask &mask, const cadm::Vec2 &end, const SurfaceWrap wrap) {
        auto target = end;
        cadm::cadf best = 2;
        if (!wrap.u) {
            const auto u = end.x < 1 - end.x
                               ? cadm::cadf{0}
                               : cadm::cadf{1};
            if (std::abs(end.x - u) < best) {
                best = std::abs(end.x - u);
                target = {u, end.y};
            }
        }
        if (!wrap.v) {
            const auto v = end.y < 1 - end.y
                               ? cadm::cadf{0}
                               : cadm::cadf{1};
            if (std::abs(end.y - v) < best) {
                target = {end.x, v};
            }
        }
        strokeSegment(mask, end, target, wrap);
    }

    /// @brief Flood-fill the non-border cells from the first free cell and mark
    /// everything it cannot reach as outside
    /// @details Whatever the fill reaches stays inside; if the border covers
    /// the whole grid there is nothing to separate and the mask is left as is
    inline void fillOutside(TrimMask &mask, const SurfaceWrap wrap) {
        const auto seed = std::ranges::find(mask.cells, Cell::inside);
        if (seed == mask.cells.end()) {
            return;
        }
        const auto size = mask.size;
        std::vector reached(mask.cells.size(), false);
        const auto seedIndex = static_cast<int>(std::distance(mask.cells.begin(), seed));
        reached[seedIndex] = true;
        std::vector stack{seedIndex};

        while (!stack.empty()) {
            const int index = stack.back();
            stack.pop_back();
            const int x = index % size;
            const int y = index / size;
            const auto visit = [&](int nx, int ny) {
                if (nx < 0 || nx >= size) {
                    if (!wrap.u) {
                        return;
                    }
                    nx = (nx + size) % size;
                }
                if (ny < 0 || ny >= size) {
                    if (!wrap.v) {
                        return;
                    }
                    ny = (ny + size) % size;
                }
                const auto n = static_cast<std::size_t>(ny) * size + nx;
                if (reached[n] || mask.cells[n] == Cell::border) {
                    return;
                }
                reached[n] = true;
                stack.push_back(static_cast<int>(n));
            };
            visit(x - 1, y);
            visit(x + 1, y);
            visit(x, y - 1);
            visit(x, y + 1);
        }

        for (std::size_t i = 0; i < mask.cells.size(); ++i) {
            if (mask.cells[i] != Cell::border && !reached[i]) {
                mask.cells[i] = Cell::outside;
            }
        }
    }

    /// @brief Build a @p size x @p size mask classifying the parameter square's
    /// cells against the curve @p params
    /// @param params curve points in the surface's (u, v)
    /// @param closed whether the curve's last point joins back to its first
    /// @param wrap whether the surface is periodic in each parameter
    /// @param size grid resolution per axis
    [[nodiscard]] inline TrimMask buildTrimMask(
        const std::vector<cadm::Vec2> &params,
        const bool closed,
        const SurfaceWrap wrap,
        const int size = 256
    ) {
        if (params.size() < 2 || size < 2) {
            return {};
        }
        TrimMask mask{
            .size = size,
            .cells = std::vector(static_cast<std::size_t>(size) * size, Cell::inside)
        };

        for (std::size_t i = 1; i < params.size(); ++i) {
            strokeSegment(mask, params[i - 1], params[i], wrap);
        }
        if (closed) {
            strokeSegment(mask, params.back(), params.front(), wrap);
        }
        else if (!wrap.u || !wrap.v) {
            extendToEdge(mask, params.front(), wrap);
            extendToEdge(mask, params.back(), wrap);
        }

        fillOutside(mask, wrap);
        return mask;
    }
}

#endif //CAD_TRIMMASK_HXX
