//
// Created by Radosław Głasek on 02.08.2026
//

#ifndef CAD_SURFACE_HXX
#define CAD_SURFACE_HXX

#include <cmath>
#include <functional>
#include <numbers>
#include <optional>

#include "BezierUtils.hpp"
#include "components/Entity.hpp"
#include "components/GeometryComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/geometry/PatchComponent.hxx"

namespace intersections {
    /// @brief Surface point and partial derivatives with respect to the global
    /// (u, v)
    struct SurfaceEval {
        cadm::Vec3 p, du, dv;
    };

    /// @brief Any surface the intersection code can march over
    /// @details Erases the concrete component type so a patch and a torus look
    /// the same to the tracer
    struct Surface {
        std::function<std::optional<SurfaceEval>(cadm::cadf u, cadm::cadf v)> eval;

        /// @brief Whether the domain is periodic in each parameter
        bool wrapU = false, wrapV = false;

        std::optional<SurfaceEval> operator()(const cadm::cadf u, const cadm::cadf v) const {
            return eval(u, v);
        }
    };

    /// @brief Joined Bézier patch
    /// @note Control points are already world-space
    [[nodiscard]] inline Surface patchSurface(const PatchComponent *patch) {
        return {
            .eval = [patch](const cadm::cadf u, const cadm::cadf v) -> std::optional<SurfaceEval> {
                const auto local = patch->resolveUv(u, v);
                const auto grid = patch->patchAtUv(u, v);
                if (!local || !grid) {
                    return std::nullopt;
                }
                const auto e = bezierUtils::bezierValueAndDerivativesAt(
                    grid.value(),
                    local->localU,
                    local->localV
                );
                // chain rule scales local derivatives to the global parameters
                return SurfaceEval{
                    .p = e[0],
                    .du = e[1] * static_cast<cadm::cadf>(patch->getPatchCountY()),
                    .dv = e[2] * static_cast<cadm::cadf>(patch->getPatchCountX()),
                };
            },
            .wrapU = patch->getWrap() == WrapDirection::u,
            .wrapV = patch->getWrap() == WrapDirection::v,
        };
    }

    [[nodiscard]] inline Surface torusSurface(const TorusComponent *torus, const cadm::Mat4 &model) {
        return {
            .eval = [torus, model](const cadm::cadf u, const cadm::cadf v) -> std::optional<SurfaceEval> {
                constexpr auto twoPi = cadm::cadf{2} * std::numbers::pi_v<cadm::cadf>;
                const auto bigR = torus->getMajorRadius();
                const auto smallR = torus->getMinorRadius();

                const auto twoPiU = u * twoPi;
                const auto twoPiV = v * twoPi;
                const auto cosU = std::cos(twoPiU),
                           sinU = std::sin(twoPiU);
                const auto cosV = std::cos(twoPiV),
                           sinV = std::sin(twoPiV);
                const auto ring = bigR + smallR * cosV;

                const cadm::Vec3 p{
                    ring * cosU,
                    smallR * sinV,
                    ring * sinU
                };
                // chain rule
                const cadm::Vec3 du{
                    -ring * sinU * twoPi,
                    0,
                    ring * cosU * twoPi
                };
                const cadm::Vec3 dv{
                    -smallR * sinV * cosU * twoPi,
                    smallR * cosV * twoPi,
                    -smallR * sinV * sinU * twoPi,
                };

                const auto transformed = [&model](const cadm::Vec3 &vec, const cadm::cadf w) {
                    const auto r = model * cadm::Vec4{vec.x, vec.y, vec.z, w};
                    return cadm::Vec3{r.x, r.y, r.z};
                };
                return SurfaceEval{
                    .p = transformed(p, 1),
                    .du = transformed(du, 0),
                    .dv = transformed(dv, 0),
                };
            },
            .wrapU = true,
            .wrapV = true,
        };
    }

    /// @brief Build a @ref Surface for whichever surface component @p e carries
    /// @returns <tt>std::nullopt</tt> if @p e is not a surface
    [[nodiscard]] inline std::optional<Surface> surfaceFor(Entity *e) {
        if (const auto patch = e->getComponent<PatchComponent>()) {
            return patchSurface(patch.value());
        }
        if (const auto torus = e->getComponent<TorusComponent>()) {
            const auto transform = e->getComponent<TransformComponent>();
            return torusSurface(
                torus.value(),
                transform
                    ? transform.value()->getModelMatrix()
                    : cadm::Mat4::identity()
            );
        }
        return std::nullopt;
    }
}

#endif //CAD_SURFACE_HXX
