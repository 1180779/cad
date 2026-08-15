//
// Created by Radosław Głasek on 15.08.2026
//

#ifndef CAD_PATCHPLACEMENTSTRATEGY_HXX
#define CAD_PATCHPLACEMENTSTRATEGY_HXX

#include "ICursorPlacementStrategy.hpp"
#include "Scene.hpp"
#include "components/geometry/PatchComponent.hxx"

class PatchPlacementStrategy final : public IViewportPositionStrategy {
public:
    explicit PatchPlacementStrategy(Scene *const scene)
    : m_scene{scene} {}

    std::optional<cadm::Vec3> resolve(
        QMouseEvent *event,
        int viewportW,
        int viewportH,
        const cadm::Mat4 &invView,
        const cadm::Mat4 &invProj
    ) override;

private:
    [[nodiscard]] std::vector<PatchComponent*> getSelectedPatches() const;

    struct NearestHit {
        cadm::cadf t = std::numeric_limits<cadm::cadf>::infinity();
        PatchComponent *patch = nullptr;
        cadm::Vec3 hit{};
        cadm::Vec2 uv{};
    };

    std::optional<NearestHit> roughResolve(cadm::Vec3 origin, cadm::Vec3 dir);

    static std::optional<cadm::Vec3> fineResolve(cadm::Vec3 origin, cadm::Vec3 dir, const NearestHit &best);

    static constexpr int gridNodes = 25;
    static constexpr cadm::cadf gridStep = 1.0F / (gridNodes - 1);
    static constexpr int subdivisions = 2;
    static constexpr cadm::cadf initStep = 0.25F;
    static constexpr cadm::cadf boxTolerance = 0.15F;
    static constexpr std::size_t minParallelSamples = 128;

    Scene *const m_scene;
};

#endif //CAD_PATCHPLACEMENTSTRATEGY_HXX
