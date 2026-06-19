//
// Created on 3/31/26.
//

#include "GridPlanePlacementStrategy.hpp"

#include <cad_math/helpers.hpp>
#include <cad_math/vec3.hpp>

namespace {
    struct PlaneCandidate {
        cadm::vec3 normal{};
        cadm::cadf offset{};
        int bit{};
    };
}

std::optional<cadm::vec3> GridPlanePlacementStrategy::resolve(
    QMouseEvent *event,
    const int viewportW,
    const int viewportH,
    const cadm::mat4 &invView,
    const cadm::mat4 &invProj
) {
    const cadm::mat4 invVP = invView * invProj;
    const cadm::vec2i screenPos(event->pos().x(), event->pos().y());
    const cadm::ray4 ray = cadm::unprojectRay(screenPos, -1.0, invVP, viewportW, viewportH);

    const cadm::vec3 origin(ray.origin.x, ray.origin.y, ray.origin.z);
    const cadm::vec3 dir(ray.direction.x, ray.direction.y, ray.direction.z);

    constexpr PlaneCandidate planes[3] = {
        {cadm::vec3(0, 0, 1), 0.0, /*XY*/ 1 << 0},
        {cadm::vec3(0, 1, 0), 0.0, /*XZ*/ 1 << 1},
        {cadm::vec3(1, 0, 0), 0.0, /*YZ*/ 1 << 2},
    };

    cadm::cadf bestAlignment = -1.0;
    const PlaneCandidate *bestPlane = nullptr;

    for (const auto &p : planes) {
        if (!(m_gridPlanesMask & p.bit)) {
            continue;
        }
        if (const cadm::cadf alignment = std::abs(p.normal.dot(dir));
            alignment > bestAlignment) {
            bestAlignment = alignment;
            bestPlane = &p;
        }
    }

    if (!bestPlane) {
        return std::nullopt;
    }

    const auto t = cadm::intersectRayPlane(origin, dir, bestPlane->normal, bestPlane->offset, s_parallelThreshold);
    if (!t.has_value()) {
        return std::nullopt;
    }

    const cadm::vec3 hit = origin + dir * t.value();
    return hit;
}
