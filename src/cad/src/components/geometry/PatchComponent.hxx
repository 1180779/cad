//
// Created by Radosław Głasek on 02.07.2026
//

#ifndef CAD_PATCHCOMPONENT_HXX
#define CAD_PATCHCOMPONENT_HXX

#include <array>
#include <ranges>
#include <set>
#include <vector>

#include "../GeometryComponent.hpp"
#include "../IPointReferrer.hpp"
#include "GpuBuffer.hpp"
#include "PointRegistry.hpp"
#include "SinglePatchView.hxx"
#include "WrapDirection.hxx"
#include "Vao.hxx"
#include "utils/TrimMask.hxx"

/// @brief Shared base for bicubic joined Bézier patches
///
/// Holds the control-point grid (row-major, @ref m_rows x @ref m_cols), the
/// per-single-patch surface index buffer (16 control points each) and the
/// control-net wireframe. Subclasses only decide how the 16 surface vertices
/// per patch are produced
///
/// Control points cannot be added/removed; the whole patch is created and
/// deleted as a unit together with its points
class PatchComponent : public GeometryComponent, public IPointReferrer {
public:
    explicit PatchComponent(PointRegistry *registry);

    ~PatchComponent() override;

    /// @brief Set the generated control-point grid
    /// @param handles  grid points, row-major (size == rows * cols)
    /// @param rows,cols  grid dimensions
    /// @param wrap  for a cylinder, the periodic direction (seam shared, the
    /// grid must not repeat it)
    /// @param patchCountX,patchCountY  number of single patches along columns / rows
    void setGrid(
        std::vector<PointHandle> handles,
        int rows,
        int cols,
        WrapDirection wrap,
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

    [[nodiscard]] WrapDirection getWrap() const {
        return m_wrap;
    }

    [[nodiscard]] int getPatchCountX() const {
        return m_patchCountX;
    }

    [[nodiscard]] int getPatchCountY() const {
        return m_patchCountY;
    }

    void replaceControlPoint(PointHandle from, PointHandle to) override;

    [[nodiscard]] std::vector<PointHandle> controlPointHandles() const override {
        return m_controlPoints;
    }

    void setControlPointHandles(const std::vector<PointHandle> &handles) override;

    /// @brief Result of mapping global (u, v) to a single patch
    struct PatchUv {
        int xPatch, yPatch;
        cadm::cadf localU, localV;
    };

    /// @brief Map global (u, v) to a single patch and local coordinates.
    /// Wraps whichever parameter @ref m_wrap marks as periodic;
    /// @returns <tt>std::nullopt</tt> when a non-wrapped parameter is outside
    /// [0, 1]
    [[nodiscard]] std::optional<PatchUv> resolveUv(cadm::cadf u, cadm::cadf v) const;

    virtual ::std::optional<bezierUtils::Grid4x4> patchAtUv(cadm::cadf u, cadm::cadf v) const = 0;

    /// @brief View of a single patch (px, py)
    [[nodiscard]] SinglePatchView singlePatch(int px, int py) const;

    /// @brief All single-patch coordinates, row-major: <tt>for (const auto [py,
    /// px] : patchCoords())</tt>
    [[nodiscard]] auto patchCoords() const {
        return std::views::cartesian_product(
            std::views::iota(0, m_patchCountY),
            std::views::iota(0, m_patchCountX)
        );
    }

    /// @brief Single-patch selection, keyed by @p index
    [[nodiscard]] bool isPatchSelected(const int index) const {
        return m_selectedPatches.contains(index);
    }

    void setPatchSelected(int index, bool selected);

    [[nodiscard]] const std::set<int>& getSelectedPatches() const {
        return m_selectedPatches;
    }

    void clearPatchSelection() {
        m_selectedPatches.clear();
    }

    [[nodiscard]] int getGridDivisionsU() const {
        return m_gridDivisionsU;
    }

    [[nodiscard]] int getGridDivisionsV() const {
        return m_gridDivisionsV;
    }

    void setGridDivisionsU(int divisions);

    void setGridDivisionsV(int divisions);

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

    /// @brief CPU copy of the surface index buffer
    [[nodiscard]] const std::vector<uint32_t>& getPatchIndices() const {
        return m_patchEbo.data();
    }

    /// @brief CPU-side position of surface vertex @p index
    [[nodiscard]] virtual cadm::Vec3 patchVertexPos(const uint32_t index) const {
        return m_registry->getPosition(index);
    }

    [[nodiscard]] GLuint getNetVao() const {
        return m_netVao.id();
    }

    [[nodiscard]] int getNetIndexCount() const {
        return m_netEbo.size();
    }

    /// @brief Hide the half of this surface that an intersection curve cut away
    /// @param mask the surface's parameter square, split along the curve
    /// @param keepInside which of the two regions stays visible
    void setTrim(trimming::TrimMask mask, const bool keepInside) {
        m_trim.set(std::move(mask), keepInside);
        m_trimDirty = true;
    }

    void clearTrim() {
        m_trim.clear();
        m_trimDirty = true;
    }

    [[nodiscard]] const trimming::TrimState& getTrim() const {
        return m_trim;
    }

    /// @brief GL texture holding the trim mask, uploaded on demand by the
    /// renderer; 0 until then
    [[nodiscard]] GLuint getTrimTexture() const {
        return m_trimTexture;
    }

    /// @brief Upload @ref getTrim's mask to @ref getTrimTexture if it changed
    /// @details Called by the renderer, which is the only place with a current
    /// GL context
    void syncTrimToGpu() const;

    /// @brief Recompute the surface vertex positions after a control point
    /// moved
    void regenerateMesh() override {}

    void syncToGpu() override;

protected:
    /// @brief Array index into @ref m_controlPoints for grid cell (row, col),
    /// wrapping whichever direction @ref m_wrap marks as the seam
    /// @pre @p row and @p col must be non-negative
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

    void buildNetEbo();

    PointRegistry *m_registry;
    std::vector<PointHandle> m_controlPoints;
    int m_rows = 0;
    int m_cols = 0;
    WrapDirection m_wrap = WrapDirection::none;
    int m_patchCountX = 0;
    int m_patchCountY = 0;

    std::set<int> m_selectedPatches;

    int m_gridDivisionsU = 4;
    int m_gridDivisionsV = 4;
    bool m_showNet = false;

    CallbackId m_positionCallbackId = -1;

    Vao m_patchVao;
    Vao m_netVao;
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_patchEbo;
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_netEbo;

    trimming::TrimState m_trim;
    /// @brief GPU mirror of @ref m_trim
    /// @note Refreshed lazily
    mutable GLuint m_trimTexture = 0;
    mutable bool m_trimDirty = false;
};

#endif //CAD_PATCHCOMPONENT_HXX
