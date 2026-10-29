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
#include "cnc/ShankCollision.hxx"
#include "cnc/program/NcProgram.hxx"
#include "cad_graphics/ShaderProgram.hxx"

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
        /// @pre A current openGl context must exist
        /// @param material Cutting material configuration
        /// @param shaderDirectory Directory containing shader sources
        /// @param error Optional failure description
        [[nodiscard]] bool initialize(
            const Material &material,
            const std::string &shaderDirectory,
            std::string *error = nullptr
        );

        /// @brief Release Gl objects
        /// @note A current context is required
        void shutdown();

        /// @brief Clear the height texture to a flat top surface
        void reset();

        /// @brief Apply cutting moves at [first, first + count)
        /// Rapids are ignored. The moves are internally split into batches; and
        /// each batch is one render pass over the affected texel rectangle
        /// @returns The number of cutting moves consumed
        std::size_t apply(
            const ToolPath &moves,
            std::size_t first,
            std::size_t count
        );

        /// @brief Apply all cutting moves, batching them
        /// @returns The number of cutting moves consumed
        std::size_t applyAll(const ToolPath &moves);

        /// @brief Read the texture to CPU
        [[nodiscard]] std::vector<float> readHeights() const;
        /// @brief Read the texture region to CPU
        [[nodiscard]] std::vector<float> readHeights(HeightRegion region) const;
        [[nodiscard]] GLuint texture() const;
        [[nodiscard]] const Material &material() const;
        [[nodiscard]] bool initialized() const;
        [[nodiscard]] std::size_t batchSize() const;

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
            const ToolPath &moves,
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
        Material m_material;
        bool m_initialized{};

        GLuint m_textures[2]{};
        GLuint m_fbos[2]{};
        std::size_t m_front{};

        cadg::ShaderProgram m_program;
        GLuint m_vao{};

        using shaderNameTuple = std::tuple<GLint*, std::string_view>;
        std::size_t m_batchSize{gc_defaultBatchSize};
    };
}

#endif //CAD_GPUHEIGHTMAP_HXX
