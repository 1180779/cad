//
// Created by Radosław Głasek on 09.07.2026
//

#include "HoleFinder.hxx"

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <utility>

#include "Scene.hpp"
#include "components/geometry/PatchC0Component.hxx"

namespace holeFinder {namespace {
        /// @brief Endpoint pair identifying an edge
        using EdgeKey = std::pair<PointHandle, PointHandle>;
        using namespace bezierUtils;

        struct EdgeRecord {
            Entity *entity{};
            HandleCurve4 boundary{};
            HandleCurve4 inner{};
            int useCount{};
        };

        EdgeKey keyOf(const HandleCurve4 &boundary) {
            return std::minmax(boundary[0], boundary[3]);
        }

        HandleCurve4 reversed(const HandleCurve4 &v) {
            return {v[3], v[2], v[1], v[0]};
        }

        /// @brief Orient @p e so its boundary row starts at @p from
        HoleEdge oriented(const EdgeRecord &e, const PointHandle from) {
            if (e.boundary[0] == from) {
                return {e.entity, e.boundary, e.inner};
            }
            return {e.entity, reversed(e.boundary), reversed(e.inner)};
        }

        std::map<EdgeKey, EdgeRecord> buildEdgeMap(Scene &scene) {
            std::map<EdgeKey, EdgeRecord> edges;
            for (Entity *e : scene.getSelectedEntities()) {
                const auto patch = e->getComponent<PatchC0Component>();
                if (!patch) {
                    continue;
                }
                for (const auto [py, px] : patch.value()->patchCoords()) {
                    const auto view = patch.value()->singlePatch(px, py);
                    const auto boundaries = view.edges();
                    const auto inners = view.innerEdges();
                    for (int i = 0; i < 4; ++i) {
                        const auto key = keyOf(boundaries[i]);
                        if (key.first == key.second) {
                            // degenerate edge
                            continue;
                        }
                        auto &[entity, boundary, inner, useCount] = edges[key];
                        entity = e;
                        boundary = boundaries[i];
                        inner = inners[i];
                        ++useCount;
                    }
                }
            }
            return edges;
        }

        std::multimap<PointHandle, const EdgeRecord*> buildAdjacencyMap(const std::map<EdgeKey, EdgeRecord> &edges) {
            std::multimap<PointHandle, const EdgeRecord*> adjacency;
            for (const auto &record : edges | std::views::values) {
                if (record.useCount != 1) {
                    continue;
                }
                adjacency.emplace(record.boundary[0], &record);
                adjacency.emplace(record.boundary[3], &record);
            }
            return adjacency;
        }
    }

    std::vector<AnySizeHole> findHoles(Scene &scene, const std::size_t cycleLength) {
        const std::map<EdgeKey, EdgeRecord> edges = buildEdgeMap(scene);
        const std::multimap<PointHandle, const EdgeRecord*> adjacency = buildAdjacencyMap(edges);

        const auto otherEnd = [](const EdgeRecord *e, const PointHandle from) {
            return e->boundary[0] == from
                       ? e->boundary[3]
                       : e->boundary[0];
        };

        std::vector<AnySizeHole> holes;
        /// a cycle is identified by its (sorted) edge set, independent of the
        /// corner it was discovered from
        std::set<std::vector<const EdgeRecord*>> seen;
        std::vector<const EdgeRecord*> pathEdges;
        std::vector<PointHandle> pathCorners;

        const auto constructHole = [&](const PointHandle start) {
            AnySizeHole hole;
            PointHandle from = start;
            for (const EdgeRecord *e : pathEdges) {
                hole.push_back(oriented(*e, from));
                from = otherEnd(e, from);
            }
            return hole;
        };

        const auto tryToSaveHole = [&](const PointHandle start, const EdgeRecord *edge) {
            pathEdges.push_back(edge);
            auto key = pathEdges;
            std::ranges::sort(key);
            if (seen.insert(std::move(key)).second) {
                holes.push_back(constructHole(start));
            }
            pathEdges.pop_back();
        };

        using AdjacencyIt = std::multimap<PointHandle, const EdgeRecord*>::const_iterator;
        struct Frame {
            AdjacencyIt it;
            AdjacencyIt end;
        };
        const auto frameOf = [&](const PointHandle corner) {
            const auto [begin, end] = adjacency.equal_range(corner);
            return Frame{begin, end};
        };

        const auto discoverHolesFromVertexDfs = [&](const PointHandle start) {
            pathCorners = {start};
            std::vector stack{frameOf(start)};
            while (!stack.empty()) {
                auto &[it, end] = stack.back();
                if (it == end) {
                    stack.pop_back();
                    pathCorners.pop_back();
                    if (!pathEdges.empty()) {
                        pathEdges.pop_back();
                    }
                    continue;
                }
                const EdgeRecord *edge = (it++)->second;
                if (std::ranges::contains(pathEdges, edge)) {
                    continue;
                }
                const PointHandle next = otherEnd(edge, pathCorners.back());
                if (pathEdges.size() + 1 == cycleLength) {
                    if (next != start) {
                        continue;
                    }
                    tryToSaveHole(start, edge);
                    continue;
                }
                if (next == start || std::ranges::contains(pathCorners, next)) {
                    continue;
                }
                pathEdges.push_back(edge);
                pathCorners.push_back(next);
                stack.push_back(frameOf(next));
            }
        };

        for (auto it = adjacency.begin(); it != adjacency.end(); it = adjacency.upper_bound(it->first)) {
            discoverHolesFromVertexDfs(it->first);
        }
        return holes;
    }
}
