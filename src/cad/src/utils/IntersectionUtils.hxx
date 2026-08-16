//
// Created by Radosław Głasek on 26.07.2026
//

#ifndef CAD_INTERSECTIONUTILS_HXX
#define CAD_INTERSECTIONUTILS_HXX

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

#include "BezierUtils.hpp"
#include "cad_math/Mat2.hxx"
#include "utils/Surface.hxx"

namespace intersections {
    using namespace bezierUtils;

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

    /// @brief Parameter-space distance between two points of one surface
    /// @param s the surface
    /// @param uv1,uv2 two parameter pairs
    [[nodiscard]] inline cadm::cadf paramSpaceDist(
        const Surface &s,
        const cadm::Vec2 &uv1,
        const cadm::Vec2 &uv2
    ) {
        auto gap = uv1 - uv2;
        if (s.wrapU) {
            gap.x -= std::floor(gap.x);
            gap.x = std::min(std::abs(gap.x), std::abs(1 - gap.x));
        }
        if (s.wrapV) {
            gap.y -= std::floor(gap.y);
            gap.y = std::min(std::abs(gap.y), std::abs(1 - gap.y));
        }
        return gap.length();
    }

    /// @brief Find a common point of two surfaces by minimizing the squared
    /// distance f(u1, v1, u2, v2) = |P1 - P2|^2 with nonlinear conjugate
    /// gradients
    /// (https://en.wikipedia.org/wiki/Nonlinear_conjugate_gradient_method)
    /// @param s1,s2 the two surfaces (may be the same)
    /// @param x0 starting parameters (u1, v1, u2, v2)
    /// @param tolerance accept when f < tolerance
    /// @param maxIterations iteration cap before giving up
    /// @param betaF conjugate-direction weight (default: Polak-Ribiere+)
    /// @param lineSearch step-length search (default: backtracking/Armijo)
    /// @param minSeparation when > 0, adds a barrier around the self
    /// intersection diagonal (u1, v1) = (u2, v2), where the objective is zero
    /// for @p s1 == @p s2; (keeps the iterates from collapsing onto the trivial
    /// solution)
    /// @returns the parameters (u1, v1, u2, v2) of the common point, or
    /// <tt>std::nullopt</tt> when the search failed to reach @p tolerance
    [[nodiscard]] inline std::optional<cadm::Vec4> nonlinearConjugateGradient(
        const Surface &s1,
        const Surface &s2,
        const cadm::Vec4 x0 = {0.5, 0.5, 0.5, 0.5},
        const cadm::cadf tolerance = 1e-10,
        const int maxIterations = 200,
        const BetaFn &betaF = polakRibiere,
        const LineSearchFn &lineSearch = backtrackingLineSearch,
        const cadm::cadf minSeparation = 0
    ) {
        constexpr auto inf = std::numeric_limits<cadm::cadf>::infinity();

        const auto diagonalGap = [&](const cadm::Vec4 &x) -> cadm::Vec2 {
            auto gap = cadm::Vec2{x.x - x.z, x.y - x.w};
            if (s1.wrapU) {
                gap.x -= std::round(gap.x);
            }
            if (s1.wrapV) {
                gap.y -= std::round(gap.y);
            }
            return gap;
        };
        const auto barrierValue = [&](const cadm::Vec4 &x) -> cadm::cadf {
            if (minSeparation <= 0) {
                return 0;
            }
            const auto d = diagonalGap(x).length();
            if (d >= minSeparation) {
                return 0;
            }
            const auto h = minSeparation - d;
            return h * h;
        };
        const auto barrierGradient = [&](const cadm::Vec4 &x) -> cadm::Vec2 {
            if (minSeparation <= 0) {
                return cadm::Vec2{};
            }
            const auto gap = diagonalGap(x);
            const auto d = gap.length();
            if (d <= cadm::gc_eps || d >= minSeparation) {
                return cadm::Vec2{};
            }
            const auto h = minSeparation - d;
            const auto dir = gap / d;
            return -2 * h * dir;
        };

        const ObjectiveFn f = [&](const cadm::Vec4 &x) -> cadm::cadf {
            const auto e1 = s1(x.x, x.y);
            const auto e2 = s2(x.z, x.w);
            if (!e1 || !e2) {
                return inf;
            }
            const auto r = e1->p - e2->p;
            return r.dot(r) + barrierValue(x);
        };

        struct ValueAndGradientPair {
            cadm::cadf value;
            cadm::Vec4 gradient;
        };
        const auto valueAndGradient =
            [&](const cadm::Vec4 &x) -> std::optional<ValueAndGradientPair> {
            const auto e1 = s1(x.x, x.y);
            const auto e2 = s2(x.z, x.w);
            if (!e1 || !e2) {
                return std::nullopt;
            }
            const auto r = e1->p - e2->p;
            const auto barrier = barrierGradient(x);
            const cadm::Vec4 grad{
                cadm::cadf{2} * r.dot(e1->du) + barrier.x,
                cadm::cadf{2} * r.dot(e1->dv) + barrier.y,
                cadm::cadf{-2} * r.dot(e2->du) - barrier.x,
                cadm::cadf{-2} * r.dot(e2->dv) - barrier.y,
            };
            return ValueAndGradientPair{
                .value = r.dot(r) + barrierValue(x),
                .gradient = grad
            };
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
            fx = fg.value().value;
            g = fg.value().gradient;

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

    /// @brief Parameters of the point on @p s nearest @p target (point
    /// projection)
    [[nodiscard]] inline cadm::Vec2 projectToSurface(
        const Surface &s,
        const cadm::Vec3 &target,
        const cadm::Vec2 start,
        const int maxIterations = 100
    ) {
        constexpr auto inf = std::numeric_limits<cadm::cadf>::infinity();
        // 2D objective embedded in the 4D signature (z, w unused) so the
        // 4D backtracking line search can be reused as is
        const ObjectiveFn f = [&](const cadm::Vec4 &x) -> cadm::cadf {
            const auto e = s(x.x, x.y);
            if (!e) {
                return inf;
            }
            const auto r = e->p - target;
            return r.dot(r);
        };

        auto uv = start;
        for (int iter = 0; iter < maxIterations; ++iter) {
            const auto e = s(uv.x, uv.y);
            if (!e) {
                break;
            }
            const auto r = e->p - target;
            const cadm::Vec2 g{
                cadm::cadf{2} * r.dot(e->du),
                cadm::cadf{2} * r.dot(e->dv),
            };
            const auto slope = -g.dot(g);
            if (slope >= -cadm::gc_eps10) {
                break;
            }
            const cadm::Vec4 x{uv.x, uv.y, 0, 0},
                             s_{-g.x, -g.y, 0, 0};
            const auto fx = r.dot(r);
            const auto alpha = backtrackingLineSearch(f, x, s_, fx, slope);
            if (alpha <= 0) {
                break;
            }
            uv = uv - alpha * g;
        }
        return uv;
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
        const Surface &s1,
        const Surface &s2,
        const cadm::Vec4 &x
    ) {
        const auto e1 = s1(x.x, x.y);
        const auto e2 = s2(x.z, x.w);
        if (!e1 || !e2) {
            return std::nullopt;
        }
        const auto n1 = e1->du.cross(e1->dv);
        const auto n2 = e2->du.cross(e2->dv);
        return n1.cross(n2).safeNormalized(cadm::Vec3{});
    }

    /// @brief Rate of change of a surface's (u, v) as the curve advances along
    /// @p tangent by one unit of arclength
    /// @returns (du/ds, dv/ds); <tt>std::nullopt</tt> at a degenerate point
    /// (du, dv parallel, so the tangent plane collapses to a line)
    [[nodiscard]] inline std::optional<cadm::Vec2> parameterVelocity(
        const cadm::Vec3 &du,
        const cadm::Vec3 &dv,
        const cadm::Vec3 &tangent
    ) {
        // the intersection curve lies on both surfaces:
        // γ(s) = P_1(u_1(s), v_1(s)) = P_2(u_2(s), v_2(s))
        // differentiating with respect to s and using dγ/ds = tangent (holds
        // since s is arclength, so curve has unit speed) gives:
        // tangent = du/ds * du + dv/ds * dv
        // (a, b) solving
        // tangent = a * du + b * dv
        // are parameter velocities (du/ds, dv/ds)
        //
        // Representing that in matrix notation as A*x = b with x = [a, b]^T and
        // A a 3x2 matrix, and expanding we get
        // | du.x  dv.x | |a|   | tangent.x |
        // | du.y  dv.y | | | = | tangent.y |
        // | du.z  dv.z | |b|   | tangent.z |
        //
        // one more equation than necessary; any two of the three rows would do
        // but which two is data-dependent. One way to avoid the selection is to
        // multiply both sides by A^T
        // A^T * A * x = A^T * b
        // [du dv]^T * [du dv] * [a b]^T = [du dv]^T * tangent
        // ^^^^^^^^^^^^^^^^^^^
        // Gram matrix of (du, dv)
        // 
        // after expansion
        // | du [dot] du  du [dot] dv | |a| = | du [dot] tangent |
        // | du [dot] dv  dv [dot] dv | |b|   | dv [dot] tangent |
        // 

        const auto gram = cadm::Mat2::symmetric(du.dot(du), du.dot(dv), dv.dot(dv));
        return gram.solveCramer(cadm::Vec2{du.dot(tangent), dv.dot(tangent)});
    }

    /// @brief Predict the parameters one @p step of arclength further along @p
    /// tangent, as a starting guess for the Newton
    /// @returns @p x unchanged if either surface is degenerate at @p x
    [[nodiscard]] inline cadm::Vec4 predictNextParameters(
        const Surface &s1,
        const Surface &s2,
        const cadm::Vec4 &x,
        const cadm::Vec3 &tangent,
        const cadm::cadf step
    ) {
        const auto e1 = s1(x.x, x.y);
        const auto e2 = s2(x.z, x.w);
        if (!e1 || !e2) {
            return x;
        }
        const auto velocity1 = parameterVelocity(e1->du, e1->dv, tangent);
        const auto velocity2 = parameterVelocity(e2->du, e2->dv, tangent);
        if (!velocity1 || !velocity2) {
            return x;
        }
        return x + cadm::Vec4{velocity1->x, velocity1->y, velocity2->x, velocity2->y} * step;
    }

    /// @brief One step along the intersection curve
    /// @param s1,s2 the two surfaces (may be the same)
    /// @param xStart initial guess in combined parameter space
    /// @param pPrev previous point on the curve, in 3D
    /// @param tangent tangent direction to march along
    /// @param step approximate arclength to move along @p tangent
    /// @param tolerance convergence threshold on the summed absolute residual,
    /// relative to the surface's distance from the origin
    /// @param xTolerance convergence threshold on the summed absolute
    /// correction @p delta
    /// @param maxIterations safety cap on iterations
    /// @param solve linear solve used each iteration, see @ref Solve4x4Fn
    /// @returns the next point's parameters (u1, v1, u2, v2) on the
    /// intersection curve, or <tt>std::nullopt</tt> if either surface's domain
    /// is left, or the method fails to converge within @p maxIterations
    [[nodiscard]] inline std::optional<cadm::Vec4> newtonRapson(
        const Surface &s1,
        const Surface &s2,
        const cadm::Vec4 &xStart,
        const cadm::Vec3 &pPrev,
        const cadm::Vec3 &tangent,
        const cadm::cadf step,
        const cadm::cadf tolerance = cadm::gc_eps10,
        const cadm::cadf xTolerance = cadm::gc_eps,
        const int maxIterations = 20,
        const Solve4X4Fn &solve = solve4X4Gepp
    ) {
        auto x = xStart;
        for (int iter = 0; iter < maxIterations; ++iter) {
            // F = [F0, F1, F2, F3]
            // F0 = surface1Point.x - surface2Point.x    |
            // F1 = surface1Point.y - surface2Point.y    | i.e., surface1Point = surface2Point (within tolerance)
            // F2 = surface1Point.z - surface2Point.z    |
            // F3 = (surface1Point - pPrev) [dot] tangent - step
            //      i.e., the new point is step further along the tangent from
            //      previous point (within tolerance)

            // 1. evaluate F(x) and check function convergence
            const auto e1 = s1(x.x, x.y);
            const auto e2 = s2(x.z, x.w);
            if (!e1 || !e2) {
                return std::nullopt;
            }
            const auto r = e1->p - e2->p;
            const cadm::Vec4 f{r.x, r.y, r.z, (e1->p - pPrev).dot(tangent) - step};
            if (const auto scale = std::max<cadm::cadf>(1, e1->p.length());
                f.absSum() <= tolerance * scale) {
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
    /// (u,v) points, and evaluate the 3D points on @p s1
    [[nodiscard]] inline IntersectionCurveData extractCurveData(
        const Surface &s1,
        const IntersectionCurve &curve
    ) {
        IntersectionCurveData data;
        data.points3D.reserve(curve.params.size());
        data.params1.reserve(curve.params.size());
        data.params2.reserve(curve.params.size());
        for (const auto &p : curve.params) {
            const auto e1 = s1(p.x, p.y);
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
    /// @param s1,s2 the two surfaces (may be the same)
    /// @param seed a starting point on the intersection
    /// @param step approximate arclength between consecutive traced points
    /// @param tolerance Newton-Rapson convergence threshold
    /// @param maxPoints safety cap per direction
    [[nodiscard]] inline IntersectionCurve traceIntersectionCurve(
        const Surface &s1,
        const Surface &s2,
        const cadm::Vec4 &seed,
        const cadm::cadf step = 0.01,
        const cadm::cadf tolerance = cadm::gc_eps10,
        const int maxPoints = 2000
    ) {
        const auto seedEval1 = s1(seed.x, seed.y);
        if (!seedEval1) {
            return {
                .params = {seed},
                .closed = false
            };
        }
        const auto seedPoint = seedEval1->p;
        const auto initialTangent = intersectionTangent(s1, s2, seed);
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
                // predict a step ahead, then correct back onto the curve
                const auto predicted = predictNextParameters(s1, s2, x, tangent, step);
                const auto next = newtonRapson(s1, s2, predicted, p, tangent, step, tolerance);
                if (!next) {
                    break;
                }
                const auto nextEval = s1(next->x, next->y);
                if (!nextEval) {
                    break;
                }
                // converged somewhere implausible for one step
                if ((nextEval->p - p).length() > step * 2) {
                    break;
                }
                if (!pts.empty() && (nextEval->p - seedPoint).length() < step) {
                    closed = true;
                    break;
                }
                pts.push_back(next.value());
                x = next.value();
                p = nextEval->p;
                const auto nextTangent = intersectionTangent(s1, s2, x);
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
        if (forwardClosed) {
            forward.insert(forward.begin(), seed);
            return {
                .params = std::move(forward),
                .closed = true
            };
        }

        auto [backward, backwardClosed] = march(-initialTangent.value());
        std::ranges::reverse(backward);
        backward.push_back(seed);
        if (!backwardClosed) {
            backward.insert(backward.end(), forward.begin(), forward.end());
        }
        return {
            .params = std::move(backward),
            .closed = backwardClosed
        };
    }

    /// @brief Whether two traced curves lie on the same 3D branch
    /// @param s evaluation used to lift the combined params to 3D
    /// @param a,b the traced curves (may be the same surface, i.e. params into
    /// @p s only)
    /// @param tol 3D distance below which a point counts as covered
    /// @param coverage fraction of points (per curve) that must be covered
    /// @param stride subsample every @p stride point to make comparison cheaper
    [[nodiscard]] inline bool areCurvesDuplicate(
        const Surface &s,
        const IntersectionCurve &a,
        const IntersectionCurve &b,
        const cadm::cadf tol,
        const double coverage = 0.9,
        const int stride = 2
    ) {
        const auto cloud = [&](const IntersectionCurve &c) {
            std::vector<cadm::Vec3> points;
            points.reserve(c.params.size() / static_cast<std::size_t>(stride) + 1);
            for (std::size_t i = 0; i < c.params.size(); i += static_cast<std::size_t>(stride)) {
                if (const auto e = s(c.params[i].x, c.params[i].y)) {
                    points.push_back(e->p);
                }
            }
            return points;
        };
        const auto pa = cloud(a);
        const auto pb = cloud(b);
        if (pa.empty() || pb.empty()) {
            return false;
        }
        const auto tol2 = tol * tol;
        // whether the fraction of from whose nearest to point lies within tol,
        // with an early bailout. The scan is brute-force O(n*m), but should be
        // fine at subsampled trace sizes; change later if this is a bottleneck
        const auto covered = [tol2, coverage](
            const std::vector<cadm::Vec3> &from,
            const std::vector<cadm::Vec3> &to
        ) {
            const auto needed = static_cast<std::size_t>(
                std::ceil(coverage * static_cast<double>(from.size()))
            );
            const auto allowedMisses = from.size() - needed;
            std::size_t near = 0;
            std::size_t misses = 0;
            for (const auto &p : from) {
                bool hit = false;
                for (const auto &q : to) {
                    if ((p - q).lengthSquared() <= tol2) {
                        hit = true;
                        break;
                    }
                }
                if (hit) {
                    if (++near >= needed) {
                        return true;
                    }
                }
                else if (++misses > allowedMisses) {
                    return false;
                }
            }
            return near >= needed;
        };
        return covered(pa, pb) && covered(pb, pa);
    }

    struct TraceOptions {
        cadm::cadf step = 0.01;
        cadm::cadf tolerance = cadm::gc_eps10;
        int maxPoints = 2000;
        cadm::cadf duplicateTolerance = 0.01;
        double duplicateCoverage = 0.9;
    };

    /// @brief Trace every branch in @p seeds, returning one
    /// <tt>IntersectionCurve</tt> per distinct physical branch
    /// @param s1,s2 the two surfaces
    /// @param seeds candidate starting points (usually from <tt>findSeeds</tt>)
    /// @param opts options; see <tt>TraceOptions</tt>
    [[nodiscard]] inline std::vector<IntersectionCurve> traceAllBranches(
        const Surface &s1,
        const Surface &s2,
        const std::vector<cadm::Vec4> &seeds,
        const TraceOptions &opts = {}
    ) {
        std::vector<IntersectionCurve> branches;
        branches.reserve(seeds.size());
        const auto duplicateTolerance = std::max(
            opts.duplicateTolerance,
            cadm::cadf{2} * opts.step
        );
        for (const auto &seed : seeds) {
            auto curve = traceIntersectionCurve(s1, s2, seed, opts.step, opts.tolerance, opts.maxPoints);
            if (curve.params.size() < 2) {
                continue;
            }
            const auto isDuplicate = std::ranges::any_of(
                branches,
                [&](const IntersectionCurve &b) {
                    return areCurvesDuplicate(
                        s1,
                        b,
                        curve,
                        duplicateTolerance,
                        opts.duplicateCoverage
                    );
                }
            );
            if (!isDuplicate) {
                branches.push_back(std::move(curve));
            }
        }
        return branches;
    }

    struct SeedOptions {
        /// @brief Grid resolution per parameter
        int samplesPerAxis = 24;

        /// @brief Cap on how many candidates are handed to the gradient search
        int maxStarts = 150;

        /// @brief Minimum wrap-aware parameter-space distance between the
        /// surface's points; set to > 0 for a self-intersection
        cadm::cadf minSeparation = 0;

        /// @brief Seeds/recovered starts closer than this in parameter space
        /// are treated as the same candidate
        cadm::cadf duplicateDistance = 0.15f;

        /// @brief Passed through to <tt>nonlinearConjugateGradient</tt>
        cadm::cadf tolerance = 1e-10;
    };

    /// @brief Sample @p s on a @p perAxis x @p perAxis grid over [0,1]^2
    /// @returns the (u, v) of each sample that evaluated, with its 3D point
    [[nodiscard]] inline std::vector<std::pair<cadm::Vec2, cadm::Vec3>> sampleSurface(
        const Surface &s,
        const int perAxis
    ) {
        std::vector<std::pair<cadm::Vec2, cadm::Vec3>> out;
        out.reserve(static_cast<std::size_t>(perAxis) * perAxis);
        for (int i = 0; i < perAxis; ++i) {
            for (int j = 0; j < perAxis; ++j) {
                const cadm::cadf u = (static_cast<cadm::cadf>(i) + cadm::cadf{0.5})
                    / static_cast<cadm::cadf>(perAxis);
                const cadm::cadf v = (static_cast<cadm::cadf>(j) + cadm::cadf{0.5})
                    / static_cast<cadm::cadf>(perAxis);

                if (const auto e = s(u, v)) {
                    out.emplace_back(cadm::Vec2{u, v}, e->p);
                }
            }
        }
        return out;
    }

    /// @brief Find starting points on the intersection of @p s1 and @p s2
    /// @param s1,s2 the two surfaces (may be the same)
    /// @param options see <tt>SeedOptions</tt>; set <tt>minSeparation</tt> when
    /// @p s1 and @p s2 are the same
    /// @returns candidate starting points
    [[nodiscard]] inline std::vector<cadm::Vec4> findSeeds(
        const Surface &s1,
        const Surface &s2,
        const SeedOptions &options = {}
    ) {
        const auto samples1 = sampleSurface(s1, options.samplesPerAxis);
        const auto samples2 = sampleSurface(s2, options.samplesPerAxis);

        struct Candidate {
            cadm::cadf distance;
            cadm::Vec4 x;
        };
        std::vector<Candidate> candidates;
        candidates.reserve(samples1.size() * samples2.size());
        for (const auto &[uv1, p1] : samples1) {
            for (const auto &[uv2, p2] : samples2) {
                if (paramSpaceDist(s1, uv1, uv2) < options.minSeparation) {
                    continue;
                }
                candidates.push_back(
                    {
                        .distance = (p1 - p2).length(),
                        .x = {uv1.x, uv1.y, uv2.x, uv2.y}
                    }
                );
            }
        }
        std::ranges::sort(candidates, {}, &Candidate::distance);

        const auto farFromAll = [&options](const std::vector<cadm::Vec4> &seen, const cadm::Vec4 &x) {
            return std::ranges::none_of(
                seen,
                [&](const cadm::Vec4 &other) {
                    return (other - x).length() < options.duplicateDistance;
                }
            );
        };

        std::vector<cadm::Vec4> starts;
        for (const auto &[distance, x] : candidates) {
            if (static_cast<int>(starts.size()) >= options.maxStarts) {
                break;
            }
            if (farFromAll(starts, x)) {
                starts.push_back(x);
            }
        }

        std::vector<cadm::Vec4> seeds;
        for (const auto &start : starts) {
            const auto seed = nonlinearConjugateGradient(
                s1,
                s2,
                start,
                options.tolerance,
                200,
                polakRibiere,
                backtrackingLineSearch,
                options.minSeparation
            );
            if (!seed) {
                continue;
            }
            if (const auto dist = paramSpaceDist(s1, {seed->x, seed->y}, {seed->z, seed->w});
                dist < options.minSeparation) {
                continue;
            }
            if (farFromAll(seeds, seed.value())) {
                seeds.push_back(seed.value());
            }
        }
        return seeds;
    }

    /// @brief Single seed for the branch nearest @p target
    /// @param s1,s2 the two surfaces (may be the same)
    /// @param target world-space point to start near
    /// @param options options; see <tt>SeedOptions</tt>
    [[nodiscard]] inline std::optional<cadm::Vec4> findSeedNear(
        const Surface &s1,
        const Surface &s2,
        const cadm::Vec3 &target,
        const SeedOptions &options = {}
    ) {
        const auto pickClearance = options.minSeparation > 0
                                       ? cadm::cadf{0.2}
                                       : cadm::cadf{0};
        const auto nearest = [&](
            const Surface &s,
            const std::optional<cadm::Vec2> &avoid
        ) -> std::optional<cadm::Vec2> {
            const auto samples = sampleSurface(s, options.samplesPerAxis);
            std::optional<cadm::Vec2> best;
            cadm::cadf bestDistance = 0;
            for (const auto &[uv, p] : samples) {
                if (avoid && paramSpaceDist(s, uv, *avoid) < pickClearance) {
                    continue;
                }
                if (const auto distance = (p - target).lengthSquared();
                    !best || distance < bestDistance) {
                    best = uv;
                    bestDistance = distance;
                }
            }
            return best;
        };
        const auto uv1 = nearest(s1, std::nullopt);
        if (!uv1) {
            return std::nullopt;
        }
        const auto uv2 = nearest(s2, uv1);
        if (!uv2) {
            return std::nullopt;
        }

        // refine coarse grid candidates with projection
        const auto p1 = projectToSurface(s1, target, *uv1);
        auto p2 = projectToSurface(s2, target, *uv2);
        if (paramSpaceDist(s1, p1, p2) < options.minSeparation) {
            // both projections collapsed onto one point of a self-intersecting
            // surface; keep the grid pick
            p2 = *uv2;
        }
        const cadm::Vec4 x0{p1.x, p1.y, p2.x, p2.y};
        const auto seed = nonlinearConjugateGradient(
            s1,
            s2,
            x0,
            options.tolerance,
            200,
            polakRibiere,
            backtrackingLineSearch,
            options.minSeparation
        );
        if (!seed.has_value()) {
            return std::nullopt;
        }
        if (const auto dist = paramSpaceDist(s1, {seed->x, seed->y}, {seed->z, seed->w});
            dist < options.minSeparation) {
            return std::nullopt;
        }
        return seed;
    }
}

#endif //CAD_INTERSECTIONUTILS_HXX
