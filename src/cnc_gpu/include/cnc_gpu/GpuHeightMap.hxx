//
// Created by Radosław Głasek on 18.10.2026
//

#ifndef CAD_GPUHEIGHTMAP_HXX
#define CAD_GPUHEIGHTMAP_HXX

#include <cstddef>
#include <string>
#include <vector>

#include <QOpenGLFunctions_4_5_Core>

#include "cnc/Material.hxx"
#include "../../../cnc/include/cnc/program/NcProgram.hxx"

namespace cnc::gpu {
    class GpuHeightMap final {
    public:
        GpuHeightMap() = default;
        ~GpuHeightMap();

        GpuHeightMap(const GpuHeightMap &) = delete;
        GpuHeightMap &operator=(const GpuHeightMap &) = delete;
        GpuHeightMap(GpuHeightMap &&) = delete;
        GpuHeightMap &operator=(GpuHeightMap &&) = delete;

        /// @brief Create the textures, FBOs, SSBO and cutter program
        /// @pre A current OpenGL 4.5 core context must exist
        /// @param material cutting material configuration
        /// @param shaderDirectory Directory containing `heightmap.vert` and
        /// `heightmap.frag`.
        /// @param error Optional failure description
        [[nodiscard]] bool initialize(
            const MaterialConfig &material,
            const std::string &shaderDirectory,
            std::string *error = nullptr
        );

        /// @brief Release GL objects. A current context is required
        void shutdown();

        /// @brief Clear the height texture to a flat top surface
        void reset();

        /// @brief Apply cutting moves at [first, first + count)
        /// Rapids are ignored. The moves are internally split into batches of
        /// batchSize(), and each batch is one render pass over the affected
        /// texel rectangle
        /// @returns The number of cutting moves consumed
        std::size_t apply(
            const std::vector<Move> &moves,
            std::size_t first,
            std::size_t count
        );

        /// @brief Apply all cutting moves, batching them
        /// @returns The number of cutting moves consumed
        std::size_t applyAll(const std::vector<Move> &moves);

        /// @brief Read the current texture back to CPU
        [[nodiscard]] std::vector<float> readHeights() const;

        [[nodiscard]] GLuint texture() const {
            return m_textures[m_front];
        }

        [[nodiscard]] const MaterialConfig &material() const {
            return m_material;
        }

        [[nodiscard]] bool initialized() const {
            return m_initialized;
        }

        [[nodiscard]] std::size_t batchSize() const {
            return m_batchSize;
        }

        /// @brief Segment count per render pass. Clamped. 
        void setBatchSize(std::size_t batchSize);

    private:
        static constexpr int gc_minBatchSize{1};
        static constexpr int gc_maxBatchSize{1024};
        static constexpr int gc_defaultBatchSize{256};
        
        bool createResources(std::string *error);

        bool compileProgram(const std::string &shaderDirectory, std::string *error);

        void destroyResources();

        /// @brief Apply one batch. Returns the number of cutting moves consumed
        std::size_t applyBatch(
            const std::vector<Move> &moves,
            std::size_t first,
            std::size_t count
        );

        [[nodiscard]] bool computeBounds(
            const Move &move,
            int &i0,
            int &i1,
            int &j0,
            int &j1
        ) const;

        QOpenGLFunctions_4_5_Core *m_gl{};
        MaterialConfig m_material;
        bool m_initialized{};

        GLuint m_textures[2]{};
        GLuint m_fbos[2]{};
        std::size_t m_front{};

        GLuint m_program{};
        GLuint m_vao{};

        using shaderNameTuple = std::tuple<GLint*, std::string_view>;
        const std::array<shaderNameTuple, 10> m_uniformNames = std::to_array<shaderNameTuple>({
            {&m_uStockSize, "uStockSize"},
            {&m_uStockWMinX, "uStockWMinX"},
            {&m_uStockWMinY, "uStockWMinY"},
            {&m_uStockCellWResolutionX, "uStockCellWResolutionX"},
            {&m_uStockCellWResolutionY, "uStockCellWResolutionY"},
            {&m_uStockWBottom, "uStockWBottom"},
            {&m_uCutterWFrom, "uCutterWFrom"},
            {&m_uCutterWTo, "uCutterWTo"},
            {&m_uCutterRadius, "uCutterRadius"},
            {&m_uCutterType, "uCutterType"},
        });
        
        GLint m_uStockSize{-1};
        GLint m_uStockWMinX{-1};
        GLint m_uStockWMinY{-1};
        GLint m_uStockCellWResolutionX{-1};
        GLint m_uStockCellWResolutionY{-1};
        GLint m_uStockWBottom{-1};
        GLint m_uCutterWFrom{-1};
        GLint m_uCutterWTo{-1};
        GLint m_uCutterRadius{-1};
        GLint m_uCutterType{-1};

        std::size_t m_batchSize{gc_defaultBatchSize};
    };
}

#endif //CAD_GPUHEIGHTMAP_HXX
