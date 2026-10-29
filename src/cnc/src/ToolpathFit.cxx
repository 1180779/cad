//
// Toolpath bounds and stock fitting helpers.
//

#include "cnc/ToolpathFit.hxx"

#include <algorithm>
#include <limits>

namespace cnc
{
    std::optional<ToolpathBounds> toolpathBounds(const Program& program)
    {
        if (program.moves.empty())
        {
            return std::nullopt;
        }

        ToolpathBounds bounds{
            .minX = std::numeric_limits<float>::max(),
            .maxX = std::numeric_limits<float>::lowest(),
            .minY = std::numeric_limits<float>::max(),
            .maxY = std::numeric_limits<float>::lowest(),
            .minZ = std::numeric_limits<float>::max(),
            .maxZ = std::numeric_limits<float>::lowest(),
        };
        for (const Move& move : program.moves)
        {
            bounds.minX = std::min({bounds.minX, move.from.x, move.to.x});
            bounds.maxX = std::max({bounds.maxX, move.from.x, move.to.x});
            bounds.minY = std::min({bounds.minY, move.from.y, move.to.y});
            bounds.maxY = std::max({bounds.maxY, move.from.y, move.to.y});
            bounds.minZ = std::min({bounds.minZ, move.from.z, move.to.z});
            bounds.maxZ = std::max({bounds.maxZ, move.from.z, move.to.z});
        }
        return bounds;
    }

    Material fitMaterialToProgram(
        const Program& program,
        const Material& base
    )
    {
        Material result = base.normalized();
        const auto bounds = toolpathBounds(program);
        if (!bounds)
        {
            return result;
        }

        const float margin = std::max(2.0f, program.tool.radius() * 2.0f);
        result.width = bounds->maxX - bounds->minX + 2.0f * margin;
        result.depth = bounds->maxY - bounds->minY + 2.0f * margin;
        result.centerX = (bounds->minX + bounds->maxX) * 0.5f;
        result.centerY = (bounds->minY + bounds->maxY) * 0.5f;
        result.topZ = bounds->maxZ;
        result.thickness = std::max(1.0f, bounds->maxZ - bounds->minZ + margin);
        return result.normalized();
    }
}
