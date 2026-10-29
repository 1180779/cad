//
// Toolpath bounds and stock fitting helpers.
//

#ifndef CNC_TOOLPATHFIT_HXX
#define CNC_TOOLPATHFIT_HXX

#include <optional>

#include "cnc/Material.hxx"
#include "program/NcProgram.hxx"

namespace cnc
{
    struct ToolpathBounds
    {
        float minX{};
        float maxX{};
        float minY{};
        float maxY{};
        float minZ{};
        float maxZ{};
    };

    [[nodiscard]] std::optional<ToolpathBounds> toolpathBounds(const Program& program);

    /// @brief Fit and centre a stock around a program's toolpath bounds
    /// Subdivision counts are taken from @p base
    [[nodiscard]] Material fitMaterialToProgram(
        const Program& program,
        const Material& base = {}
    );
}

#endif //CNC_TOOLPATHFIT_HXX
