//
// Created by Radosław Głasek on 02.07.2026
//

#ifndef CAD_PATCHC0COMPONENT_HXX
#define CAD_PATCHC0COMPONENT_HXX

#include "PatchComponent.hxx"

/// @brief Bicubic joined Bézier patch with C0 continuity. Grid points are the
/// Bézier control points themselves, so the surface VAO binds the registry
/// position VBO directly and needs no CPU vertex conversion
class PatchC0Component final : public PatchComponent {
public:
    explicit PatchC0Component(PointRegistry *registry)
    : PatchComponent(registry) {}

    std::optional<bezierUtils::Grid4x4> patchAtUv(cadm::cadf u, cadm::cadf v) const override;

protected:
    [[nodiscard]] int patchRowBase(const int py) const override {
        return 3 * py;
    }

    [[nodiscard]] int patchColBase(const int px) const override {
        return 3 * px;
    }

    void rebuildPatchData() override;

    [[nodiscard]] GLuint patchSourceVbo() const override {
        return m_registry->getPositionVBO();
    }
};

#endif //CAD_PATCHC0COMPONENT_HXX
