//
// Created by Radosław Głasek on 09.07.2026
//

#ifndef CAD_GREGORYCOMPONENT_HXX
#define CAD_GREGORYCOMPONENT_HXX

#include <vector>

#include "../GeometryComponent.hpp"
#include "../IPointReferrer.hpp"
#include "GpuBuffer.hpp"
#include "PointRegistry.hpp"
#include "Vao.hxx"
#include "utils/GregoryUtils.hxx"

/// @brief Gregory patch filling a hole between C0 Bézier patches
///
/// References the hole's existing control points; owns no points. The derived n
/// x 20 Gregory net vertices (layout per <tt>gregory::Net</tt>) are recomputed
/// via <tt>gregory::fillHole</tt> whenever a referenced point moves
class GregoryComponent final : public GeometryComponent, public IPointReferrer {
public:
    static constexpr int s_handlesPerEdge = 8;

    explicit GregoryComponent(PointRegistry *registry);

    ~GregoryComponent() override;

    /// @brief Set the hole definition
    /// @param handles flat list, <tt>s_handlesPerEdge</tt> per edge (boundary
    /// row then inner row, both oriented along the hole cycle); size must be a
    /// non-zero multiple of it
    void setHole(std::vector<PointHandle> handles);

    [[nodiscard]] int netCount() const {
        return static_cast<int>(m_handles.size()) / s_handlesPerEdge;
    }

    void replaceControlPoint(PointHandle from, PointHandle to) override;

    [[nodiscard]] std::vector<PointHandle> controlPointHandles() const override {
        return m_handles;
    }

    void setControlPointHandles(const std::vector<PointHandle> &handles) override;

    /// @brief Subdivisions of net @p net along U
    [[nodiscard]] int getGridDivisionsU(const int net) const {
        return m_gridDivisionsU[net];
    }

    /// @brief Subdivisions of net @p net along V
    [[nodiscard]] int getGridDivisionsV(const int net) const {
        return m_gridDivisionsV[net];
    }

    [[nodiscard]] const std::vector<int>& gridDivisionsU() const {
        return m_gridDivisionsU;
    }

    [[nodiscard]] const std::vector<int>& gridDivisionsV() const {
        return m_gridDivisionsV;
    }

    void setGridDivisionsU(int net, int divisions);

    void setGridDivisionsV(int net, int divisions);

    [[nodiscard]] bool getShowVectors() const {
        return m_showVectors;
    }

    void setShowVectors(const bool v) {
        m_showVectors = v;
    }

    [[nodiscard]] GLuint getVectorsVao() const {
        return m_vectorsVao.id();
    }

    [[nodiscard]] int getVectorsIndexCount() const {
        return m_vectorsEbo.size();
    }

    [[nodiscard]] GLuint getPatchVao() const {
        return m_patchVao.id();
    }

    /// @brief Total surface index count
    [[nodiscard]] int getPatchIndexCount() const {
        return m_ebo.size();
    }

    /// @brief CPU-side position of surface vertex @p index
    [[nodiscard]] cadm::Vec3 patchVertexPos(const uint32_t index) const {
        return m_vbo[static_cast<int>(index)];
    }

    /// @brief Recompute the Gregory nets from the current point positions
    void regenerateMesh() override;

    void syncToGpu() override;

private:
    /// @brief Resolve the flat handle list to per-edge position rows
    [[nodiscard]] std::vector<gregory::EdgeData> edgeData() const;

    PointRegistry *m_registry;

    /// @brief Flat hole handles, @ref s_handlesPerEdge per edge
    std::vector<PointHandle> m_handles;

    /// @brief Per-net subdivisions
    std::vector<int> m_gridDivisionsU;
    std::vector<int> m_gridDivisionsV;
    bool m_showVectors = false;

    CallbackId m_positionCallbackId = -1;

    Vao m_patchVao;
    Vao m_vectorsVao;
    GpuBuffer<cadm::Vec3, GL_ARRAY_BUFFER> m_vbo;
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_ebo;

    /// @brief Line list over <tt>m_vbo</tt>: net ring + the cross-tangent
    /// (continuity) vectors
    GpuBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER> m_vectorsEbo;
};

#endif //CAD_GREGORYCOMPONENT_HXX
