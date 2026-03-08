//
// Created on 3/7/26.
//

#ifndef CAD_HELPERS_H
#define CAD_HELPERS_H

#include "mat4.h"
#include "ray4.h"
#include "vec2.h"
#include "vec2i.h"
#include "vec4.h"

namespace cadm
{
    // Unprojects a 2D screen point with a given NDC depth z to World Space.
    // z should be in the range [-1, 1] (OpenGL) or [0, 1] (DirectX/Vulkan) depending on the projection matrix.
    // Top left is the (0, 0) point.
    inline vec4 unproject(const vec2i point, const cadf z, const mat4& invWorldPV, const int width, const int height)
    {
        const cadf halfWidth = static_cast<cadf>(width / 2.0);
        const cadf halfHeight = static_cast<cadf>(height / 2.0);

        const vec2 ndcPoint((static_cast<cadf>(point.x) - halfWidth) / halfWidth,
                            (halfHeight - static_cast<cadf>(point.y)) / halfHeight);

        vec4 unprojectedPoint(ndcPoint.x, ndcPoint.y, z, 1.0f);
        unprojectedPoint = invWorldPV * unprojectedPoint;
        unprojectedPoint /= unprojectedPoint.w;
        return unprojectedPoint;
    }

    // Unprojects a 2D screen point with a given NDC depth z to World Space ray.
    // z should be the lower value (-1 for OpenGL or 1 for DirectX/Vulkan) depending on the projection matrix.
    // Top left is the (0, 0) point.
    inline ray4 unprojectRay(const vec2i point, const cadf zNear, const mat4& invWorldPV, const int width,
                             const int height)
    {
        const cadf halfWidth = static_cast<cadf>(width / 2.0);
        const cadf halfHeight = static_cast<cadf>(height / 2.0);

        const vec2 ndcPoint((static_cast<cadf>(point.x) - halfWidth) / halfWidth,
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
}

#endif //CAD_HELPERS_H
