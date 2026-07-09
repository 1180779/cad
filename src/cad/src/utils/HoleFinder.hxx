//
// Created by Radosław Głasek on 09.07.2026
//

#ifndef CAD_HOLEFINDER_HXX
#define CAD_HOLEFINDER_HXX

#include <algorithm>
#include <array>
#include <span>
#include <vector>

#include "components/geometry/SinglePatchView.hxx"

class Entity;
class Scene;

namespace holeFinder {
    /// @brief One boundary edge of a hole: a single-patch boundary row and the
    /// row adjacent to it
    struct HoleEdge {
        /// @brief The entity owning the edge
        Entity *entity{};

        /// @brief Boundary row, oriented along the hole cycle
        bezierUtils::HandleCurve4 boundary{};

        /// @brief Adjacent inner row, same orientation
        bezierUtils::HandleCurve4 inner{};
    };

    /// @brief A closed @p N sided hole between patches. Edges chain:
    /// <tt>edges[i].boundary[3] == edges[(i + 1) % N].boundary[0]</tt>
    template <std::size_t N> requires (N >= 3)
    struct Hole {
        std::array<HoleEdge, N> edges{};
    };

    using AnySizeHole = std::vector<HoleEdge>;

    /// @brief Flatten a hole into <tt>GregoryComponent</tt>'s handle layout: 8
    /// handles per edge, boundary row then inner row, edges in cycle order
    inline std::vector<PointHandle> flatHandles(const std::span<const HoleEdge> edges) {
        std::vector<PointHandle> out;
        out.reserve(edges.size() * 8);
        for (const auto &[entity, boundary, inner] : edges) {
            const auto b = boundary.flatView();
            const auto in = inner.flatView();
            out.insert(out.end(), b.begin(), b.end());
            out.insert(out.end(), in.begin(), in.end());
        }
        return out;
    }

    /// @brief Find every closed @p cycleLength edge boundary among the
    /// currently selected C0 surfaces
    ///
    /// An edge qualifies if exactly one selected single patch uses it (shared
    /// edges are surface-interior); holes are simple cycles of that length in
    /// the corner-handle graph of those edges (found by DFS)
    std::vector<AnySizeHole> findHoles(Scene &scene, std::size_t cycleLength);

    /// @brief Fixed-size facade over the runtime-length search
    /// @see <tt>findHoles(Scene&, std::size_t)</tt>
    template <std::size_t N> requires (N >= 3)
    std::vector<Hole<N>> findHoles(Scene &scene) {
        std::vector<Hole<N>> out;
        for (const auto &edges : findHoles(scene, N)) {
            Hole<N> hole;
            std::ranges::copy(edges, hole.edges.begin());
            out.push_back(hole);
        }
        return out;
    }
}

#endif //CAD_HOLEFINDER_HXX
