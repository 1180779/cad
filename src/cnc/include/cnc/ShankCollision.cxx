/*
 * Created by Radosław Głasek on 29.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#include "cnc/ShankCollision.hxx"

#include <algorithm>
#include <cmath>

namespace cnc {

std::optional<HeightRegion> shankRegion(const Material& material, const Move& move) {
    const auto radius = move.tool.radius();
    if (radius <= 0.0f ||
        std::min(move.from.z, move.to.z) + move.tool.cuttingLength() >= material.topZ) {
        return std::nullopt;
    }

    const auto min = material.minXY();
    const auto left = std::min(move.from.x, move.to.x) - radius;
    const auto right = std::max(move.from.x, move.to.x) + radius;
    const auto low = std::min(move.from.y, move.to.y) - radius;
    const auto high = std::max(move.from.y, move.to.y) + radius;
    if (right < min.x ||
        left > min.x + material.width ||
        high < min.y ||
        low > min.y + material.depth) {
        return std::nullopt;
    }

    const auto nx = material.resolutionX();
    const auto ny = material.resolutionY();
    const auto cellSize = material.cellSizeXY();
    const auto x0 = std::clamp(static_cast<int>(std::floor((left - min.x) / cellSize.x)), 0, nx - 1);
    const auto x1 = std::clamp(static_cast<int>(std::ceil((right - min.x) / cellSize.x)), 0, nx - 1);
    const auto y0 = std::clamp(static_cast<int>(std::floor((low - min.y) / cellSize.y)), 0, ny - 1);
    const auto y1 = std::clamp(static_cast<int>(std::ceil((high - min.y) / cellSize.y)), 0, ny - 1);
    return HeightRegion{
        .x = x0,
        .y = y0,
        .width = x1 - x0 + 1,
        .height = y1 - y0 + 1
    };
}

std::optional<Warning> checkShankCollision(
    const Material& material,
    const Move& move,
    const HeightRegion region,
    const std::span<const float> heights) {
    if (region.width <= 0 ||
        region.height <= 0 ||
        heights.size() != region.sampleCount()) {
        return std::nullopt;
    }

    const auto delta = move.to.xy() - move.from.xy();
    const auto lengthSq = delta.lengthSquared();
    const auto radiusSq = move.tool.radius() * move.tool.radius();
    const auto base = material.bottomZ();
    for (int y = 0; y < region.height; ++y) {
        for (int x = 0; x < region.width; ++x) {
            const auto height = heights[region.index(x, y)];
            if (height <= base + 1e-4f) {
                continue;
            }
            const auto offset = material.samplePositionXY(region.sampleCoordinates(x, y)) - move.from.xy();
            auto enter = 0.0f;
            auto leave = 1.0f;
            if (lengthSq > 1e-4f) {
                const auto projection = offset.dot(delta) / lengthSq;
                const auto distanceSq = std::max(
                    0.0f,
                    offset.lengthSquared() - projection * projection * lengthSq
                    );
                if (distanceSq >= radiusSq) {
                    continue;
                }
                const auto halfInterval = std::sqrt((radiusSq - distanceSq) / lengthSq);
                enter = std::max(0.0f, projection - halfInterval);
                leave = std::min(1.0f, projection + halfInterval);
                if (enter >= leave) {
                    continue;
                }
            } else if (offset.lengthSquared() >= radiusSq) {
                continue;
            }

            const bool descendingRapid = move.type == MoveType::G00 &&
                                         move.to.z < move.from.z;
            const auto t = descendingRapid
                               ? leave
                               : enter;
            if (const auto position = move.from.lerp(move.to, static_cast<cadm::cadf>(t));
                height > position.z + move.tool.cuttingLength() + 1e-4f) {
                return Warning{
                    .kind = WarningKind::nonCuttingPart,
                    .moveIndex = move.instructionIndex,
                    .programIndex = move.programIndex,
                    .position = position,
                    .message = "Remaining material at z="
                               + std::to_string(height)
                               + " mm overlaps the non-cutting shank (cutting length "
                               + std::to_string(move.tool.cuttingLength())
                               + " mm).",
                    .timeline = move.timeline
                };
            }
        }
    }
    return std::nullopt;
}
} // namespace cnc
