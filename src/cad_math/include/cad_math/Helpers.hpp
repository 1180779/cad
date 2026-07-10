//
// Created on 3/7/26.
//

#ifndef CAD_HELPERS_H
#define CAD_HELPERS_H

#include <optional>

#include "Mat3.hpp"
#include "Mat4.hpp"
#include "Ray4.hpp"
#include "Vec2.hpp"
#include "Vec3.hpp"
#include "Vec4.hpp"

namespace cadm {
    /// @brief Projects a 3D world position to 2D screen coordinates.
    /// Assumes OpenGL NDC convention: x and y in [-1, 1], z in [-1, 1].
    /// Returns std::nullopt if the point is behind the camera (w <= 0).
    /// Top left is the (0, 0) point
    inline std::optional<Vec2I> projectToScreenGl(
        const Vec3 &worldPos,
        const Mat4 &view,
        const Mat4 &projection,
        const int width,
        const int height
    ) {
        const auto clip = projection * view * Vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        if (clip.w <= 0.0f) {
            return std::nullopt;
        }
        const cadf ndcX = clip.x / clip.w;
        const cadf ndcY = clip.y / clip.w;
        return Vec2I(
            static_cast<int>((ndcX + 1.0f) / 2.0f * static_cast<cadf>(width)),
            static_cast<int>((1.0f - ndcY) / 2.0f * static_cast<cadf>(height))
        );
    }

    /// @brief Unprojects a 2D screen point at a specific NDC depth to a World Space position.
    /// ndcZ: the clip-space Z to unproject at (e.g. pivot depth from VP * pivotPos).
    /// Top left is the (0, 0) point
    inline Vec3 unprojectPoint(
        const Vec2I point,
        const cadf ndcZ,
        const Mat4 &invVp,
        const int width,
        const int height
    ) {
        const cadf halfWidth = static_cast<cadf>(width / 2.0);
        const cadf halfHeight = static_cast<cadf>(height / 2.0);

        const cadf ndcX = (static_cast<cadf>(point.x) - halfWidth) / halfWidth;
        const cadf ndcY = (halfHeight - static_cast<cadf>(point.y)) / halfHeight;

        Vec4 world = invVp * Vec4(ndcX, ndcY, ndcZ, 1.0);
        world /= world.w;
        return {world.x, world.y, world.z};
    }

    // @brief Unprojects a 2D screen point with a given NDC depth z to World Space ray.
    // z should be the lower value (-1 for OpenGL or 1 for DirectX/Vulkan) depending on the projection matrix.
    // Top left is the (0, 0) point
    inline Ray4 unprojectRay(
        const Vec2I point,
        const cadf zNear,
        const Mat4 &invWorldPV,
        const int width,
        const int height
    ) {
        const cadf halfWidth = static_cast<cadf>(width / 2.0);
        const cadf halfHeight = static_cast<cadf>(height / 2.0);

        const Vec2 ndcPoint(
            (static_cast<cadf>(point.x) - halfWidth) / halfWidth,
            (halfHeight - static_cast<cadf>(point.y)) / halfHeight
        );

        Vec4 unprojectedNearPoint(ndcPoint.x, ndcPoint.y, zNear, 1.0);
        unprojectedNearPoint = invWorldPV * unprojectedNearPoint;
        unprojectedNearPoint /= unprojectedNearPoint.w;

        Vec4 unprojectedFarPoint(ndcPoint.x, ndcPoint.y, 1.0, 1.0);
        unprojectedFarPoint = invWorldPV * unprojectedFarPoint;
        unprojectedFarPoint /= unprojectedFarPoint.w;

        Vec4 rayDir = (unprojectedFarPoint - unprojectedNearPoint).normalized();
        return {unprojectedNearPoint, rayDir};
    }

    // extracts ZYX Euler angles (rx, ry, rz) from rotation matrix M = Rz * Ry * Rx

    inline Vec3 eulerZYXFromRotMat(const Mat3 &m) {
        // https://en.wikipedia.org/wiki/Euler_angles
        const auto m20 = m.row(2)[0];
        const auto m00 = m.row(0)[0];
        const auto m10 = m.row(1)[0];
        const auto m21 = m.row(2)[1];
        const auto m22 = m.row(2)[2];

        const auto alpha = std::atan2(m10, m00);
        const auto beta = std::asin(-m20);
        const auto gamma = std::atan2(m21, m22);

        return {gamma, beta, alpha};
    }

    /// @brief Intersects a ray (origin + t*dir) with the infinite plane dot(normal, p) = offset.
    /// Returns the parameter t at the intersection point, or nullopt when the ray is
    /// parallel to the plane.
    /// The hit position is: origin + dir * t
    inline std::optional<cadf> intersectRayPlane(
        const Vec3 &origin,
        const Vec3 &dir,
        const Vec3 &normal,
        const cadf offset,
        const cadf parallelThreshold = gc_feps
    ) {
        const auto denom = normal.dot(dir);
        if (std::abs(denom) < parallelThreshold) {
            return std::nullopt;
        }
        return (offset - normal.dot(origin)) / denom;
    }
}

#endif //CAD_HELPERS_H
