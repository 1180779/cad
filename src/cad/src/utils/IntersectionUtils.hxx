//
// Created by Radosław Głasek on 26.07.2026
//

#ifndef CAD_INTERSECTIONUTILS_HXX
#define CAD_INTERSECTIONUTILS_HXX

#include <algorithm>
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

    [[nodiscard]] inline std::optional<cadm::Vec4> solve4X4Gepp(const cadm::Mat4 &a, const cadm::Vec4 &b) {
        return a / b;
    }

    /// @brief Signature of the linear solve used by each Newton-Raphson
    /// iteration; see @ref solve4x4.
    /// @details For more details see <br>
    /// Numerical Recipes (3rd ed.), Ch. 9.6
    /// "Newton-Raphson Method for Nonlinear Systems of Equations" <br> 
    /// They solve the same per-iteration system via LU decomposition; this
    /// method defaults to GEPP instead since the Jacobian is freshly rebuilt
    /// every iteration
    using Solve4X4Fn = std::function<std::optional<cadm::Vec4>(const cadm::Mat4 &, const cadm::Vec4 &)>;

    /// @brief Tangent direction of the intersection curve at @p x
    [[nodiscard]] inline std::optional<cadm::Vec3> intersectionTangent(
        const PatchComponent *patch1,
        const PatchComponent *patch2,
        const cadm::Vec4 &x
    ) {
        const auto e1 = evaluateSurface(patch1, x.x, x.y);
        const auto e2 = evaluateSurface(patch2, x.z, x.w);
        if (!e1 || !e2) {
            return std::nullopt;
        }
        const auto n1 = e1->du.cross(e1->dv);
        const auto n2 = e2->du.cross(e2->dv);
        return n1.cross(n2).safeNormalized(cadm::Vec3{});
    }

    /// @brief One step along the intersection curve
    /// @param patch1,patch2 the two surfaces (may be the same)
    /// @param xPrev previous point on the curve, in combined parameter space
    /// @param pPrev previous point on the curve, in 3D
    /// @param tangent tangent direction to march along
    /// @param step approximate arclength to move along @p tangent
    /// @param tolerance convergence threshold on the summed absolute residual
    /// @param xTolerance convergence threshold on the summed absolute
    /// correction @p delta
    /// @param maxIterations safety cap on iterations
    /// @param solve linear solve used each iteration, see @ref Solve4x4Fn
    /// @returns the next point's parameters (u1, v1, u2, v2) on the
    /// intersection curve, or <tt>std::nullopt</tt> if either surface's domain
    /// is left, or the method fails to converge within @p maxIterations
    [[nodiscard]] inline std::optional<cadm::Vec4> newtonRapson(
        const PatchComponent *patch1,
        const PatchComponent *patch2,
        const cadm::Vec4 &xPrev,
        const cadm::Vec3 &pPrev,
        const cadm::Vec3 &tangent,
        const cadm::cadf step,
        const cadm::cadf tolerance = 1e-10,
        const cadm::cadf xTolerance = 1e-12,
        const int maxIterations = 20,
        const Solve4X4Fn &solve = solve4X4Gepp
    ) {
        auto x = xPrev;
        for (int iter = 0; iter < maxIterations; ++iter) {
            // F = [F0, F1, F2, F3]
            // F0 = patch1point.x - patch2point.x    |
            // F1 = patch1point.y - patch2point.y    | i.e., patch1point = patch2point (within tolerance)
            // F2 = patch1point.z - patch2point.z    |
            // F3 = (patch1point - pPrev) [dot] tangent - step
            //      i.e., the new point is step further along the tangent from
            //      previous point (within tolerance)

            // 1. evaluate F(x) and check function convergence
            const auto e1 = evaluateSurface(patch1, x.x, x.y);
            const auto e2 = evaluateSurface(patch2, x.z, x.w);
            if (!e1 || !e2) {
                return std::nullopt;
            }
            const auto r = e1->p - e2->p;
            const cadm::Vec4 f{r.x, r.y, r.z, (e1->p - pPrev).dot(tangent) - step};
            if (f.absSum() <= tolerance) {
                return x;
            }

            // 2. assemble the Jacobian
            // columns: d/du1, d/dv1, d/du2, d/dv2
            // rows:    F0, F1, F2, F3
            const cadm::Mat4 j{
                cadm::Vec4{e1->du.x, e1->du.y, e1->du.z, e1->du.dot(tangent)},
                cadm::Vec4{e1->dv.x, e1->dv.y, e1->dv.z, e1->dv.dot(tangent)},
                cadm::Vec4{-e2->du.x, -e2->du.y, -e2->du.z, 0},
                cadm::Vec4{-e2->dv.x, -e2->dv.y, -e2->dv.z, 0},
            };

            // 3. solve J * delta = -F and update x
            const auto delta = solve(j, -f);
            if (!delta) {
                return std::nullopt;
            }
            const auto &deltaV = delta.value();
            x += deltaV;

            // 4. check root convergence
            if (deltaV.absSum() <= xTolerance) {
                return x;
            }
        }
        return std::nullopt;
    }

    /// @brief A traced intersection curve in the two surfaces' combined
    /// parameter space (u1, v1, u2, v2)
    struct IntersectionCurve {
        std::vector<cadm::Vec4> params;
        bool closed = false;
    };

    /// @brief Per-surface parameter-space points and 3D points of @p curve
    struct IntersectionCurveData {
        std::vector<cadm::Vec3> points3D;
        std::vector<cadm::Vec2> params1;
        std::vector<cadm::Vec2> params2;
    };

    /// @brief Split @p curve's combined (u1,v1,u2,v2) params into per-surface
    /// (u,v) points, and evaluate the 3D points on @p patch1
    [[nodiscard]] inline IntersectionCurveData extractCurveData(
        const PatchComponent *patch1,
        const IntersectionCurve &curve
    ) {
        IntersectionCurveData data;
        data.points3D.reserve(curve.params.size());
        data.params1.reserve(curve.params.size());
        data.params2.reserve(curve.params.size());
        for (const auto &p : curve.params) {
            const auto e1 = evaluateSurface(patch1, p.x, p.y);
            data.points3D.push_back(
                e1
                    ? e1->p
                    : cadm::Vec3{}
            );
            data.params1.push_back({p.x, p.y});
            data.params2.push_back({p.z, p.w});
        }
        return data;
    }

    /// @brief Trace the intersection curve through @p seed by marching with
    /// Newton-Rapson method in both directions along the curve tangent
    /// @param patch1,patch2 the two surfaces (may be the same)
    /// @param seed a starting point on the intersection
    /// @param step approximate arclength between consecutive traced points
    /// @param tolerance Newton-Rapson convergence threshold
    /// @param maxPoints safety cap per direction
    [[nodiscard]] inline IntersectionCurve traceIntersectionCurve(
        const PatchComponent *patch1,
        const PatchComponent *patch2,
        const cadm::Vec4 &seed,
        const cadm::cadf step = 0.01,
        const cadm::cadf tolerance = 1e-8,
        const int maxPoints = 2000
    ) {
        const auto seedEval1 = evaluateSurface(patch1, seed.x, seed.y);
        if (!seedEval1) {
            return {
                .params = {seed},
                .closed = false
            };
        }
        const auto seedPoint = seedEval1->p;
        const auto initialTangent = intersectionTangent(patch1, patch2, seed);
        if (!initialTangent || initialTangent->lengthSquared() < cadm::gc_eps) {
            return {
                .params = {seed},
                .closed = false
            };
        }

        const auto march = [&](const cadm::Vec3 &dir0) {
            std::vector<cadm::Vec4> pts;
            auto x = seed;
            auto p = seedPoint;
            auto tangent = dir0;
            bool closed = false;
            for (int i = 0; i < maxPoints; ++i) {
                const auto next = newtonRapson(patch1, patch2, x, p, tangent, step, tolerance);
                if (!next) {
                    break;
                }
                const auto nextEval = evaluateSurface(patch1, next->x, next->y);
                if (!nextEval) {
                    break;
                }
                if (!pts.empty() && (nextEval->p - seedPoint).length() < step * 0.5) {
                    closed = true;
                    break;
                }
                pts.push_back(next.value());
                x = next.value();
                p = nextEval->p;
                const auto nextTangent = intersectionTangent(patch1, patch2, x);
                if (!nextTangent || nextTangent->lengthSquared() < cadm::gc_eps) {
                    break;
                }
                tangent = nextTangent->dot(tangent) < 0
                              ? -nextTangent.value()
                              : nextTangent.value();
            }
            return IntersectionCurve{
                .params = pts,
                .closed = closed
            };
        };

        auto [forward, forwardClosed] = march(initialTangent.value());
        auto [backward, backwardClosed] = march(-initialTangent.value());

        std::ranges::reverse(backward);
        backward.push_back(seed);
        backward.insert(backward.end(), forward.begin(), forward.end());
        return {
            .params = std::move(backward),
            .closed = forwardClosed || backwardClosed
        };
    }
}

#endif //CAD_INTERSECTIONUTILS_HXX
