//
// Created by Radosław Głasek on 03.07.2026
//

#ifndef CAD_PATCHC2COMPONENT_HXX
#define CAD_PATCHC2COMPONENT_HXX

#include <cad_math/Vec3.hpp>

#include "PatchComponent.hxx"

/// @brief Bicubic joined B-spline patch with C2 continuity. Grid points are de
/// Boor points
class PatchC2Component final : public PatchComponent {
public:
    explicit PatchC2Component(PointRegistry *registry) : PatchComponent(registry) {}

    ~PatchC2Component() override;

    [[nodiscard]] cadm::Vec3 patchVertexPos(const uint32_t index) const override {
        return m_bernsteinVbo[static_cast<int>(index)];
    }

    void regenerateMesh() override;

protected:
    [[nodiscard]] int patchRowBase(const int py) const override {
        return py;
    }

    [[nodiscard]] int patchColBase(const int px) const override {
        return px;
    }

    void rebuildPatchData() override;

    [[nodiscard]] GLuint patchSourceVbo() const override {
        return m_bernsteinVbo.vboId();
    }

    void syncPatchVertices(QOpenGLFunctions_4_5_Core *gl) override {
        m_bernsteinVbo.syncToGpu(gl);
    }

private:
    /// @brief Derived Bernstein positions, 16 per single patch (joins
    /// duplicated)
    GpuBuffer<cadm::Vec3, GL_ARRAY_BUFFER> m_bernsteinVbo;
};

#endif //CAD_PATCHC2COMPONENT_HXX
