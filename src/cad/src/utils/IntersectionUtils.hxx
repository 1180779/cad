//
// Created by Radosław Głasek on 26.07.2026
//

#ifndef CAD_INTERSECTIONUTILS_HXX
#define CAD_INTERSECTIONUTILS_HXX

#include <functional>
#include <limits>

#include "BezierUtils.hpp"
#include "components/geometry/PatchComponent.hxx"

namespace intersections {
    using namespace bezierUtils;

    /// @brief Surface point and partial derivatives with respect to the global
    /// (u, v)
    struct SurfaceEval {
        cadm::Vec3 p, du, dv;
    };

    /// @brief Evaluate @p patch at global (@p u, @p v): point + derivatives
    /// scaled to the global parametrization (chain rule over the patch grid)
    /// @returns <tt>std::nullopt</tt> outside the (non-wrapped) domain
    [[nodiscard]] inline std::optional<SurfaceEval> evaluateSurface(
        const PatchComponent *patch,
        const cadm::cadf u,
        const cadm::cadf v
    ) {
        const auto local = patch->resolveUv(u, v);
        const auto grid = patch->patchAtUv(u, v);
        if (!local || !grid) {
            return std::nullopt;
        }
        const auto e = bezierValueAndDerivativesAt(grid.value(), local->localU, local->localV);
        return SurfaceEval{
            .p = e[0],
            .du = e[1] * static_cast<cadm::cadf>(patch->getPatchCountY()),
            .dv = e[2] * static_cast<cadm::cadf>(patch->getPatchCountX()),
        };
    }

    /// @brief Objective for the closest-point search: f(x) = |P1(x.x, x.y) -
    /// P2(x.z, x.w)|^2 (+inf outside the domains)
    using ObjectiveFn = std::function<cadm::cadf(const cadm::Vec4 &)>;

    /// @brief Conjugate-direction weight beta(gNew, gPrev, dPrev)
    using BetaFn = std::function<cadm::cadf(const cadm::Vec4 &, const cadm::Vec4 &, const cadm::Vec4 &)>;

    /// @brief Line search: alpha minimizing f(x + alpha * d)
    using LineSearchFn = std::function<cadm::cadf(
        const ObjectiveFn &f,
        const cadm::Vec4 &x,
        const cadm::Vec4 &d,
        cadm::cadf fx,
        cadm::cadf slope
    )>;

    /// @brief Polak-Ribiere+ beta
    [[nodiscard]] inline cadm::cadf polakRibiere(
        const cadm::Vec4 &gNew,
        const cadm::Vec4 &gPrev,
        const cadm::Vec4 &dPrev
    ) {
        const auto denom = gPrev.dot(gPrev);
        if (denom <= cadm::gc_eps) {
            return 0;
        }
        return std::max<cadm::cadf>(0, gNew.dot(gNew - gPrev) / denom);
    }

    /// @brief Backtracking line search with the Armijo condition
    /// @see Nocedal, Wright - "Numerical Optimization", Procedure 3.1
    /// (Backtracking Line Search), Chapter 3 "Line Search Methods"
    [[nodiscard]] inline cadm::cadf backtrackingLineSearch(
        const ObjectiveFn &f,
        const cadm::Vec4 &x,
        const cadm::Vec4 &s,
        const cadm::cadf fx,
        const cadm::cadf slope
    ) {
        static constexpr cadm::cadf armijoC = 1e-4;
        static constexpr cadm::cadf rho = 0.5;
        cadm::cadf alpha = 1.0;
        for (int i = 0; i < 40; ++i) {
            if (f(x + alpha * s) <= fx + armijoC * alpha * slope) {
                return alpha;
            }
            alpha *= rho;
        }
        return 0;
    }

    /// @brief Find a common point of two surfaces by minimizing the squared
    /// distance f(u1, v1, u2, v2) = |P1 - P2|^2 with nonlinear conjugate
    /// gradients
    /// (https://en.wikipedia.org/wiki/Nonlinear_conjugate_gradient_method)
    /// @param patch1,patch2 the two surfaces (may be the same)
    /// @param x0 starting parameters (u1, v1, u2, v2)
    /// @param tolerance accept when f < tolerance
    /// @param maxIterations iteration cap before giving up
    /// @param betaF conjugate-direction weight (default: Polak-Ribiere+)
    /// @param lineSearch step-length search (default: backtracking/Armijo)
    /// @returns the parameters (u1, v1, u2, v2) of the common point, or
    /// <tt>std::nullopt</tt> when the search failed to reach @p tolerance
    [[nodiscard]] inline std::optional<cadm::Vec4> nonlinearConjugateGradient(
        const PatchComponent *patch1,
        const PatchComponent *patch2,
        const cadm::Vec4 x0 = {0.5, 0.5, 0.5, 0.5},
        const cadm::cadf tolerance = 1e-10,
        const int maxIterations = 200,
        const BetaFn &betaF = polakRibiere,
        const LineSearchFn &lineSearch = backtrackingLineSearch
    ) {
        constexpr auto inf = std::numeric_limits<cadm::cadf>::infinity();

        const ObjectiveFn f = [&](const cadm::Vec4 &x) -> cadm::cadf {
            const auto e1 = evaluateSurface(patch1, x.x, x.y);
            const auto e2 = evaluateSurface(patch2, x.z, x.w);
            if (!e1 || !e2) {
                return inf;
            }
            const auto r = e1->p - e2->p;
            return r.dot(r);
        };

        const auto valueAndGradient =
            [&](const cadm::Vec4 &x) -> std::optional<std::pair<cadm::cadf, cadm::Vec4>> {
            const auto e1 = evaluateSurface(patch1, x.x, x.y);
            const auto e2 = evaluateSurface(patch2, x.z, x.w);
            if (!e1 || !e2) {
                return std::nullopt;
            }
            const auto r = e1->p - e2->p;
            const cadm::Vec4 grad{
                cadm::cadf{2} * r.dot(e1->du),
                cadm::cadf{2} * r.dot(e1->dv),
                cadm::cadf{-2} * r.dot(e2->du),
                cadm::cadf{-2} * r.dot(e2->dv),
            };
            return {{r.dot(r), grad}};
        };

        auto x = x0;
        cadm::cadf fx{};
        cadm::Vec4 g{}, gPrev{}, s{};

        // 1 - 5 steps from https://en.wikipedia.org/wiki/Nonlinear_conjugate_gradient_method
        for (int iter = 0; iter < maxIterations; ++iter) {
            // 1. Calculate the steepest direction
            const auto fg = valueAndGradient(x);
            if (!fg) {
                if (iter == 0) {
                    return std::nullopt;
                }
                break;
            }
            gPrev = g;
            std::tie(fx, g) = fg.value();

            if (fx < tolerance) {
                return x;
            }
            const auto deltaX = -g;

            if (iter == 0) {
                // s_0 = delta x_0
                s = deltaX;
            }
            else {
                // 2. Compute beta_n
                const auto beta = betaF(g, gPrev, s);
                // 3. Update the conjugate direction
                s = deltaX + s * beta;
            }

            // ensure slope points "downwards"
            auto slope = g.dot(s);
            if (slope >= 0) {
                s = deltaX;
                slope = g.dot(s);
                if (slope >= 0) {
                    break; // gradient ~ 0
                }
            }

            // 4. Perform a line search
            const auto alpha = lineSearch(f, x, s, fx, slope);
            if (alpha <= 0) {
                break; // no step length gave sufficient decrease
            }

            // 5. Update the position
            x = x + s * alpha;
        }
        return fx < tolerance
                   ? std::optional{x}
                   : std::nullopt;
    }
}

#endif //CAD_INTERSECTIONUTILS_HXX
