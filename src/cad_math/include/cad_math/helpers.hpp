//
// Created on 3/7/26.
//

#ifndef CAD_HELPERS_H
#define CAD_HELPERS_H

#include <optional>

#include "mat4.hpp"
#include "ray4.hpp"
#include "vec2.hpp"
#include "vec2i.hpp"
#include "vec3.hpp"
#include "vec4.hpp"

namespace cadm
{
    // Projects a 3D world position to 2D screen coordinates.
    // Assumes OpenGL NDC convention: x and y in [-1, 1], z in [-1, 1].
    // Returns std::nullopt if the point is behind the camera (w <= 0).
    // Top left is the (0, 0) point.
    inline std::optional<vec2i> projectToScreenGL(
        const vec3 &worldPos,
        const mat4 &view,
        const mat4 &projection,
        const int width,
        const int height)
    {
        const auto clip = projection * view * vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        if (clip.w <= 0.0f) return std::nullopt;
        const cadf ndcX = clip.x / clip.w;
        const cadf ndcY = clip.y / clip.w;
        return vec2i(
            static_cast<int>((ndcX + 1.0f) / 2.0f * static_cast<cadf>(width)),
            static_cast<int>((1.0f - ndcY) / 2.0f * static_cast<cadf>(height)));
    }

    // Unprojects a 2D screen point with a given NDC depth z to World Space ray.
    // z should be the lower value (-1 for OpenGL or 1 for DirectX/Vulkan) depending on the projection matrix.
    // Top left is the (0, 0) point.
    inline ray4 unprojectRay(
        const vec2i point,
        const cadf zNear,
        const mat4 &invWorldPV,
        const int width,
        const int height)
    {
        const cadf halfWidth = static_cast<cadf>(width / 2.0);
        const cadf halfHeight = static_cast<cadf>(height / 2.0);

        const vec2 ndcPoint(
            (static_cast<cadf>(point.x) - halfWidth) / halfWidth,
            (halfHeight - static_cast<cadf>(point.y)) / halfHeight);

        vec4 unprojectedNearPoint(ndcPoint.x, ndcPoint.y, zNear, 1.0);
        unprojectedNearPoint = invWorldPV * unprojectedNearPoint;
        unprojectedNearPoint /= unprojectedNearPoint.w;

        vec4 unprojectedFarPoint(ndcPoint.x, ndcPoint.y, 1.0, 1.0);
        unprojectedFarPoint = invWorldPV * unprojectedFarPoint;
        unprojectedFarPoint /= unprojectedFarPoint.w;

        vec4 rayDir = (unprojectedFarPoint - unprojectedNearPoint).normalized();
        return {unprojectedNearPoint, rayDir};
    }

    // Intersects a ray (origin + t*dir) with the infinite plane dot(normal, p) = offset.
    // Returns the parameter t at the intersection point, or nullopt when the ray is
    // parallel to the plane.
    // The hit position is: origin + dir * t.
    inline std::optional<cadf> intersectRayPlane(
        const vec3 &origin,
        const vec3 &dir,
        const vec3 &normal,
        const cadf offset,
        const cadf parallelThreshold = feps)
    {
        const auto denom = normal.dot(dir);
        if (std::abs(denom) < parallelThreshold)
            return std::nullopt;
        return (offset - normal.dot(origin)) / denom;
    }
}

#endif //CAD_HELPERS_H