/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_UI_INCLUDE_CNC_UI_CNC_RENDER_SYSTEM_HXX
#define CAD_HEADER_SRC_CNC_UI_INCLUDE_CNC_UI_CNC_RENDER_SYSTEM_HXX

#include "cad_graphics/GpuMesh.hxx"
#include "cad_graphics/ShaderProgram.hxx"

#include <memory>
#include <string>
#include <vector>

#include <cad_math/Mat4.hxx>
#include <cad_math/Vec3.hxx>

#include "cnc/Material.hxx"
#include "cnc/Snapshot.hxx"
#include "cnc_gpu/GpuHeightMap.hxx"

namespace cnc::ui
{
    struct RenderOptions
    {
        bool showPaths{true};
        bool showTool{true};
        bool flatShading{false};
    };

    class CncRenderSystem final
    {
    public:
        CncRenderSystem() = default;
        ~CncRenderSystem();

        CncRenderSystem(const CncRenderSystem&) = delete;
        CncRenderSystem& operator=(const CncRenderSystem&) = delete;

        [[nodiscard]] bool initialize(
            const std::string& shaderDirectory,
            std::string* error = nullptr
        );

        void shutdown();
        void setMaterial(const Material& material);
        void setPath(std::shared_ptr<const ToolPath> path);

        void render(
            const gpu::GpuHeightMap& heightMap,
            const Snapshot& snapshot,
            const cadm::Mat4& view,
            const cadm::Mat4& projection,
            const cadm::Vec3& camPos,
            const RenderOptions& options
        ) const;

    private:
        bool loadPrograms(const std::string& shaderDirectory, std::string* error);

        void rebuildSideMesh();
        void rebuildPathMesh();
        void createTopVao();
        void createToolMeshes();

        void drawStock(
            const gpu::GpuHeightMap& heightMap,
            const cadm::Mat4& viewProjection,
            const cadm::Vec3& camPos,
            bool flatShading
        ) const;

        void drawPaths(const cadm::Mat4& viewProjection) const;

        void drawTool(
            const Snapshot& snapshot,
            const cadm::Mat4& viewProjection,
            const cadm::Vec3& camPos,
            bool flatShading
        ) const;

        QOpenGLFunctions_4_5_Core* m_gl{};
        bool m_initialized{};

        cadg::ShaderProgram m_stockProgram;
        cadg::ShaderProgram m_sideProgram;
        cadg::ShaderProgram m_lineProgram;
        cadg::ShaderProgram m_toolProgram;

        GLuint m_topVao{};
        cadg::GpuMesh m_sideMesh;
        cadg::GpuMesh m_pathMesh;
        cadg::GpuMesh m_sphereMesh;
        cadg::GpuMesh m_cylinderMesh;

        Material m_material;
        std::shared_ptr<const ToolPath> m_path;
        bool m_materialDirty{true};
        bool m_pathDirty{true};
    };
}

#endif // CAD_HEADER_SRC_CNC_UI_INCLUDE_CNC_UI_CNC_RENDER_SYSTEM_HXX
