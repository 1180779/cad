/*
 * Created by Radosław Głasek on 29.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_INCLUDE_CNC_SHANK_COLLISION_HXX
#define CAD_HEADER_SRC_CNC_INCLUDE_CNC_SHANK_COLLISION_HXX

#include "Snapshot.hxx"
#include <optional>
#include <span>

namespace cnc {

struct HeightRegion {
    int x{}, y{}, width{}, height{};

    [[nodiscard]] std::size_t sampleCount() const {
        return static_cast<std::size_t>(width) * height;
    }

    [[nodiscard]] std::size_t index(const int localX, const int localY) const {
        return static_cast<std::size_t>(localY) * width + localX;
    }

    [[nodiscard]] cadm::Vec2I sampleCoordinates(const int localX, const int localY) const {
        return cadm::Vec2I{x, y} + cadm::Vec2I{localX, localY};
    }
};

/// @brief Possibly reached stock samples
[[nodiscard]] std::optional<HeightRegion> shankRegion(const Material& material, const Move& move);

/// @param heights Row-major data from the provided region of the GPU texture
[[nodiscard]] std::optional<Warning> checkShankCollision(
    const Material& material,
    const Move& move,
    HeightRegion region,
    std::span<const float> heights);
} // namespace cnc

#endif // CAD_HEADER_SRC_CNC_INCLUDE_CNC_SHANK_COLLISION_HXX
