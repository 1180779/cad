//
// Created by Radosław Głasek on 15.08.2026
//

#include <execution>
#include <numeric>
#include <ranges>

#include <algorithm>

#include "PatchPlacementStrategy.hxx"

#include "cad_math/Helpers.hpp"
#include "cad_math/Ray4.hpp"
#include "components/geometry/PatchComponent.hxx"
#include "utils/BezierUtils.hpp"

namespace {
    struct PatchBox {
        cadm::Vec3 min;
        cadm::Vec3 max;
        cadm::cadf maxExtent;

        [[nodiscard]] bool contains(const cadm::Vec3 &p, const cadm::cadf tolerance) const {
            const auto pad = maxExtent * tolerance;
            return p.x >= min.x - pad && p.x <= max.x + pad && p.y >= min.y - pad && p.y <= max.y + pad
                && p.z >= min.z - pad && p.z <= max.z + pad;
        }
    };

    [[nodiscard]] PatchBox patchBox(const bezierUtils::Grid4x4 &grid) {
        cadm::Vec3 minP = grid[0][0];
        cadm::Vec3 maxP = grid[0][0];
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                const auto &p = grid[i][j];
                minP.x = std::min(minP.x, p.x);
                minP.y = std::min(minP.y, p.y);
                minP.z = std::min(minP.z, p.z);
                maxP.x = std::max(maxP.x, p.x);
                maxP.y = std::max(maxP.y, p.y);
                maxP.z = std::max(maxP.z, p.z);
            }
        }
        const auto extent = maxP - minP;
        return {
            .min = minP,
            .max = maxP,
            .maxExtent = std::max({extent.x, extent.y, extent.z})
        };
    }

    struct SurfaceSample {
        cadm::Vec3 point;
        cadm::Vec3 normal;
        PatchBox box;
    };

    [[nodiscard]] std::optional<SurfaceSample> sampleSurface(
        const PatchComponent *patch,
        const cadm::cadf u,
        const cadm::cadf v
    ) {
        const auto gridOpt = patch->patchAtUv(u, v);
        if (!gridOpt.has_value()) {
            return std::nullopt;
        }
        const auto grid = gridOpt.value();
        const auto value = bezierUtils::bezierValueAndDerivativesAt(grid, u, v);
        const auto normal = value.y.cross(value.z);
        if (normal.lengthSquared() < cadm::gc_eps * cadm::gc_eps) {
            return std::nullopt;
        }
        return SurfaceSample{
            .point = value.x,
            .normal = normal.normalized(),
            .box = patchBox(grid)
        };
    }

    struct RayHit {
        cadm::cadf t;
        cadm::Vec3 point;
    };

    [[nodiscard]] std::optional<RayHit> rayHitInBox(
        const cadm::Vec3 &origin,
        const cadm::Vec3 &dir,
        const SurfaceSample &sample,
        const cadm::cadf tolerance
    ) {
        const auto tOpt = cadm::intersectRayPlane(origin, dir, sample.normal, sample.point);
        if (!tOpt.has_value() || tOpt.value() < 0) {
            return std::nullopt;
        }
        const auto hit = origin + dir * tOpt.value();
        if (!sample.box.contains(hit, tolerance)) {
            return std::nullopt;
        }
        return RayHit{
            .t = tOpt.value(),
            .point = hit
        };
    }
}

std::vector<PatchComponent*> PatchPlacementStrategy::getSelectedPatches() const {
    std::vector<PatchComponent*> selectedPatches{};
    for (const auto e : m_scene->getSelectedEntities()) {
        if (const auto component = e->getComponent<PatchComponent>();
            component.has_value()) {
            selectedPatches.push_back(component.value());
        }
    }
    return selectedPatches;
}

std::optional<PatchPlacementStrategy::NearestHit> PatchPlacementStrategy::roughResolve(
    const cadm::Vec3 origin,
    const cadm::Vec3 dir
) {
    static constexpr int nodesPerPatch = gridNodes * gridNodes;
    const auto patches = getSelectedPatches();
    const auto sampleAt = [&origin, &dir, &patches](const int sampleIndex) -> NearestHit {
        const int patchIndex = sampleIndex / nodesPerPatch;
        const int node = sampleIndex % nodesPerPatch;
        const cadm::Vec2 uv{
            static_cast<cadm::cadf>(node % gridNodes) * gridStep,
            static_cast<cadm::cadf>(node / gridNodes) * gridStep // NOLINT(*-integer-division)
        };
        PatchComponent *patch = patches[patchIndex];
        const auto surfaceSample = sampleSurface(patch, uv.x, uv.y);
        if (!surfaceSample.has_value()) {
            return {};
        }
        if (const auto hit = rayHitInBox(origin, dir, *surfaceSample, boxTolerance);
            hit.has_value()) {
            return NearestHit{
                .t = hit->t,
                .patch = patch,
                .hit = hit->point,
                .uv = uv
            };
        }
        return {};
    };
    const auto keepNearest = [](const NearestHit &a, const NearestHit &b) {
        return b.t < a.t
                   ? b
                   : a;
    };

    const int sampleCount = static_cast<int>(patches.size()) * nodesPerPatch;
    const bool usePar = static_cast<std::size_t>(sampleCount) >= minParallelSamples;
    const auto indexRange = std::views::iota(0, sampleCount);
    const auto reduce = [&](const auto policy) {
        return std::transform_reduce(
            policy,
            indexRange.begin(),
            indexRange.end(),
            NearestHit{},
            keepNearest,
            sampleAt
        );
    };
    const NearestHit best = usePar
                                ? reduce(std::execution::par)
                                : reduce(std::execution::seq);
    if (best.patch == nullptr) {
        return std::nullopt;
    }
    return best;
}

std::optional<cadm::Vec3> PatchPlacementStrategy::fineResolve(
    const cadm::Vec3 origin,
    const cadm::Vec3 dir,
    const NearestHit &best
) {
    cadm::cadf step = initStep;
    cadm::Vec2 uv = best.uv;
    cadm::cadf bestT = best.t;
    cadm::Vec3 bestHit = best.hit;
    for (int level = 0; level < subdivisions; ++level) {
        const int span = level == 0
                             ? 2
                             : 1;
        const cadm::Vec2 center{
            level == 0
                ? cadm::cadf{0.5}
                : uv.x,
            level == 0
                ? cadm::cadf{0.5}
                : uv.y
        };
        for (int iu = -span; iu <= span; ++iu) {
            for (int iv = -span; iv <= span; ++iv) {
                const cadm::Vec2 candidate{
                    center.x + static_cast<cadm::cadf>(iu) * step,
                    center.y + static_cast<cadm::cadf>(iv) * step
                };
                const auto sample = sampleSurface(best.patch, candidate.x, candidate.y);
                if (!sample.has_value()) {
                    continue;
                }
                if (const auto hit = rayHitInBox(origin, dir, sample.value(), boxTolerance);
                    hit.has_value() && hit->t < bestT) {
                    bestT = hit->t;
                    bestHit = hit->point;
                    uv = candidate;
                }
            }
        }
        step *= 0.5;
    }
    return bestHit;
}

std::optional<cadm::Vec3> PatchPlacementStrategy::resolve(
    QMouseEvent *event,
    const int viewportW,
    const int viewportH,
    const cadm::Mat4 &invView,
    const cadm::Mat4 &invProj
) {
    const cadm::Mat4 invVp = invView * invProj;
    const cadm::Vec2I screenPos(event->pos().x(), event->pos().y());
    const cadm::Ray4 ray = cadm::unprojectRay(screenPos, -1.0, invVp, viewportW, viewportH);

    const cadm::Vec3 origin(ray.origin.x, ray.origin.y, ray.origin.z);
    const cadm::Vec3 dir(ray.direction.x, ray.direction.y, ray.direction.z);

    const auto bestOpt = roughResolve(origin, dir);
    if (!bestOpt.has_value()) {
        return std::nullopt;
    }

    return fineResolve(origin, dir, bestOpt.value());
}
