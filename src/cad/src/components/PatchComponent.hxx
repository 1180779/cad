//
// Created by Radosław Głasek on 02.07.2026
//

#ifndef CAD_PATCHCOMPONENT_HXX
#define CAD_PATCHCOMPONENT_HXX

#include <array>
#include <vector>

#include "GeometryComponent.hpp"
#include "GpuBuffer.hpp"
#include "PointRegistry.hpp"
#include "Vao.hxx"

/// @brief Shared base for bicubic joined Bézier patches
///
/// Holds the control-point grid (row-major, @ref m_rows x @ref m_cols), the
/// per-single-patch surface index buffer (16 control points each) and the
/// control-net wireframe. Subclasses only decide how the 16 surface vertices
/// per patch are produced
///
/// Control points cannot be added/removed; the whole patch is created and
/// deleted as a unit together with its points
class PatchComponent : public GeometryComponent {
public:
    explicit PatchComponent(PointRegistry *registry);

    ~PatchComponent() override;

    /// @brief Set the generated control-point grid
    /// @param handles  grid points, row-major (size == rows * cols)
    /// @param rows,cols  grid dimensions
    /// @param wrapU  true for a cylinder: the column direction wraps (seam shared)
    /// @param patchCountX,patchCountY  number of single patches along columns / rows
    void setGrid(
        std::vector<PointHandle> handles,
        int rows,
        int cols,
        bool wrapU,
        int patchCountX,
        int patchCountY
    );

    [[nodiscard]] const std::vector<PointHandle>& getControlPoints() const {
        return m_controlPoints;
    }

    [[nodiscard]] int getPatchCount() const {
        return m_patchCountX * m_patchCountY;
    }

    [[nodiscard]] int getRows() const {
        return m_rows;
    }

    [[nodiscard]] int getCols() const {
        return m_cols;
    }

    [[nodiscard]] bool getWrapU() const {
        return m_wrapU;
    }

    [[nodiscard]] int getPatchCountX() const {
        return m_patchCountX;
    }

    [[nodiscard]] int getPatchCountY() const {
        return m_patchCountY;
    }

    [[nodiscard]] int getGridDivisions() const {
        return m_gridDivisions;
    }

    void setGridDivisions(int divisions);

    [[nodiscard]] bool getShowNet() const {
        return m_showNet;
    }

    void setShowNet(bool v);

    [[nodiscard]] GLuint getPatchVao() const {
        return m_patchVao.id();
    }

    /// @brief Total surface index count (16 per single patch)
    [[nodiscard]] int getPatchIndexCount() const {
        return m_patchEbo.size();
    }

    [[nodiscard]] GLuint getNetVao() const {
        return m_netVao.id();
    }

    [[nodiscard]] int getNetIndexCount() const {
        return m_netEbo.size();
    }

    void regenerateMesh() override {}

    void syncToGpu() override;

protected:
    /// @brief Array index into @ref m_controlPoints for grid cell (row, col),
    /// wrapping the column when @ref m_wrapU is set
    [[nodiscard]] int gridIndex(int row, int col) const;

    /// @brief Top-left grid cell of single patch (px, py) in (row, col) terms
    /// @related @ref patchColBase
    [[nodiscard]] virtual int patchRowBase(int py) const = 0;

    /// @brief Top-left grid cell of single patch (px, py) in (row, col) terms
    /// @related @ref patchRowBase
    [[nodiscard]] virtual int patchColBase(int px) const = 0;

    /// @brief Gather the 16 grid array-indices of single patch (px, py), laid
    /// out k = i*4 + j with i the row (u) and j the column (v) offset
    void gatherPatch(int px, int py, std::array<int, 16> &out) const;

    /// @brief Rebuild @ref m_patchEbo (and, for C2, the Bernstein vertex VBO)
    virtual void rebuildPatchData() = 0;

    /// @brief VBO the surface VAO binds
    [[nodiscard]] virtual GLuint patchSourceVbo() const = 0;

    /// @brief Upload owned surface vertex data
    virtual void syncPatchVertices(QOpenGLFunctions_4_5_Core *gl) {}

    /// @brief Recompute the surface vertex positions after a control point
    /// moved
    virtual void recomputeGeometry() {}

    void buildNetEbo();

    PointRegistry *m_registry;
    std::vector<PointHandle> m_controlPoints;
    int m_rows = 0;
    int m_cols = 0;
    bool m_wrapU = false;
    int m_patchCountX = 0;
    int m_patchCountY = 0;

    int m_gridDivisions = 4;
    bool m_showNet = false;

    CallbackId m_positionCallbackId = -1;

    Vao m_patchVao;
    Vao m_netVao;
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_patchEbo;
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_netEbo;

    /// @brief A control point moved: surface vertices need recompute
    bool m_geometryDirty = false;
};

#endif //CAD_PATCHCOMPONENT_HXX
