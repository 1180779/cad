//
// Created by Radosław Głasek on 18.10.2026
//

#ifndef CAD_MATERIAL_HXX
#define CAD_MATERIAL_HXX

#include <algorithm>

#include "NcProgram.hxx"

namespace cnc {
    /// @brief Stock and grid configuration
    ///
    /// The stock is centred on (centerX, centerY) with its top surface at
    /// z = topZ and its base at z = topZ - thickness. Subdivisions are the
    /// number of grid cells; the height map stores (subdivisions + 1) samples
    /// per axis
    struct MaterialConfig {
        float width{100.0f};
        float depth{100.0f};
        float thickness{20.0f};
        /// @brief XY centre of the stock in world coordinates
        float centerX{0.0f};
        float centerY{0.0f};
        /// @brief Z coordinate of the initial top surface
        float topZ{0.0f};
        int subdivisionsX{gc_defaultSubdivisions};
        int subdivisionsY{gc_defaultSubdivisions};

        [[nodiscard]] int resolutionX() const {
            return std::max(subdivisionsX, 1) + 1;
        }

        [[nodiscard]] int resolutionY() const {
            return std::max(subdivisionsY, 1) + 1;
        }

        static constexpr int gc_defaultSubdivisions = 512;
        static constexpr int gc_maxSubdivisions = 4096;
        static constexpr float gc_maxExt = 100000.0f;
        
        /// @brief Clamp the configuration to physically meaningful values
        [[nodiscard]] MaterialConfig normalized() const {
            MaterialConfig result = *this;
            result.width = std::clamp(width, 0.1f, gc_maxExt);
            result.depth = std::clamp(depth, 0.1f, gc_maxExt);
            result.thickness = std::clamp(thickness, 0.1f, gc_maxExt);
            result.topZ = std::clamp(topZ, -gc_maxExt, gc_maxExt);
            result.centerX = std::clamp(centerX, -gc_maxExt, gc_maxExt);
            result.centerY = std::clamp(centerY, -gc_maxExt, gc_maxExt);
            result.subdivisionsX = std::clamp(subdivisionsX, 1, gc_maxSubdivisions);
            result.subdivisionsY = std::clamp(subdivisionsY, 1, gc_maxSubdivisions);
            return result;
        }
    };
}

#endif //CAD_MATERIAL_HXX
