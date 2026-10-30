//
// Created by Radosław Głasek on 18.10.2026
//

#ifndef CAD_GPUHEIGHTMAP_HXX
#define CAD_GPUHEIGHTMAP_HXX

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cnc/Material.hxx"
#include "cnc/program/NcProgram.hxx"
#include "cad_graphics/ShaderProgram.hxx"

namespace cnc::gpu {
    class GpuHeightMap final {
    public:
        enum Issue : std::uint32_t {
            shankCollision = 1u,
            verticalPlunge = 1u << 1,
        };

        GpuHeightMap() = default;
        ~GpuHeightMap();

        GpuHeightMap(const GpuHeightMap &) = delete;
        GpuHeightMap &operator=(const GpuHeightMap &) = delete;
        GpuHeightMap(GpuHeightMap &&) = delete;
        GpuHeightMap &operator=(GpuHeightMap &&) = delete;

        [[nodiscard]] bool initialize(
            const Material &material,
            const std::string &shaderDirectory,
            std::string *error = nullptr
        );

        void shutdown();
        void reset() const;

        /// @brief Apply cutting moves at [first, first + count)
        /// @param issues one bitmask per input move
        /// @returns The number of cutting moves consumed
        std::size_t apply(
            const ToolPath &moves,
            std::size_t first,
            std::size_t count,
            std::vector<std::uint32_t>* issues = nullptr
        ) const;

        /// @brief Apply all cutting move
        /// @returns The number of cutting moves consumed
        std::size_t applyAll(const ToolPath &moves) const;

        [[nodiscard]] GLuint texture() const;
        [[nodiscard]] const Material &material() const;
        [[nodiscard]] bool initialized() const;
        [[nodiscard]] std::size_t batchSize() const;
        void setBatchSize(std::size_t batchSize);

    private:
        static constexpr int gc_minBatchSize{1};
        static constexpr int gc_maxBatchSize{1024};
        static constexpr int gc_defaultBatchSize{256};

        bool createResources(std::string *error);
        bool compileProgram(const std::string &shaderDirectory, std::string *error);
        void destroyResources();

        /// @brief Apply one batch
        /// @returns The number of cutting moves consumed
        std::size_t applyBatch(
            const ToolPath &moves,
            std::size_t first,
            std::size_t count,
            std::uint32_t* issues
        ) const;

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

        GLuint m_texture{};
        GLuint m_fbo{};
        GLuint m_issueFbo{};
        GLuint m_issueBuffer{};

        cadg::ShaderProgram m_program;
        cadg::ShaderProgram m_issueProgram;
        GLuint m_vao{};

        using shaderNameTuple = std::tuple<GLint*, std::string_view>;
        std::size_t m_batchSize{gc_defaultBatchSize};
    };
}

#endif //CAD_GPUHEIGHTMAP_HXX
