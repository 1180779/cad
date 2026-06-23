//
// Created by Radosław Głasek on 23.06.2026
//

#include "InterpC2Solver.hxx"

#include <cmath>

namespace {
    constexpr cadm::cadf gc_minChord = 1e-3f;
    constexpr cadm::cadf gc_minChordSq = gc_minChord * gc_minChord;

    /// @brief Drop coincident points so no chord has zero length
    std::vector<cadm::Vec3> dedup(std::span<const cadm::Vec3> points);

    /// @brief Per-segment knot intervals d_i = |p_{i+1} - p_i|
    std::vector<cadm::cadf> knotIntervals(const std::vector<cadm::Vec3> &p);

    /// @brief Solve the natural-spline tridiagonal system for the tau^2 coefficients
    /// c_0...c_m (ends zero). Normalized rows alpha_i*c_{i-1} + 2*c_i + beta_i*c_{i+1} = R_i,
    /// solved with the Thomas algorithm over the interior c_1...c_{m-1}
    std::vector<cadm::Vec3> solveC2Coeffs(
        const std::vector<cadm::Vec3> &p,
        const std::vector<cadm::cadf> &d
    );

    /// @brief Power basis Q_i(tau) = a_i + b_i*tau + c_i*tau^2 + d_i*tau^3 -> Bernstein per
    /// segment, shared-endpoint layout
    void toBernstein(
        const std::vector<cadm::Vec3> &p,
        const std::vector<cadm::cadf> &d,
        const std::vector<cadm::Vec3> &c,
        std::vector<cadm::Vec3> &out
    );
}

namespace interpC2 {
    void solve(const std::span<const cadm::Vec3> points, std::vector<cadm::Vec3> &out) {
        out.clear();

        const auto p = dedup(points);
        if (p.size() < 2) {
            return;
        }
        const auto d = knotIntervals(p);
        const auto c = solveC2Coeffs(p, d);

        toBernstein(p, d, c, out);
    }
}

namespace {
    void toBernstein(
        const std::vector<cadm::Vec3> &p,
        const std::vector<cadm::cadf> &d,
        const std::vector<cadm::Vec3> &c,
        std::vector<cadm::Vec3> &out
    ) {
        const std::size_t m = d.size();
        out.resize(m * 3 + 1);

        // segment Q_i spans [t_i, t_{i+1}), tau = t - t_i, with
        // Q_i(tau) = a_i + b_i*tau + c_i*tau^2 + d_i*tau^3
        //
        // c coeffs are already computed; below a, b, d are recovered from the segment
        // conditions, evaluated for segment i over [0, dI]:
        //
        //   (interp, left)      a_i = Q_i(0) = p_i
        //
        //   (C2, right end)     2*c_i + 6*d_i*dI = 2*c_{i+1}
        //                                 => d_i = (c_{i+1} - c_i) / (3*dI)
        //
        //   (interp, right)           Q_i(dI) = p_{i+1}
        //  a_i + b_i*dI + c_i*dI^2 + d_i*dI^3 = p_{i+1}
        //       solve for b_i,         b_i*dI = (p_{i+1} - p_i) - c_i*dI^2 - d_i*dI^3
        //       sub d_i,             d_i*dI^3 = (c_{i+1} - c_i)/(3*dI) * dI^3 = (c_{i+1} - c_i)*dI^2 / 3
        //                              b_i*dI = (p_{i+1} - p_i) - c_i*dI^2 - (c_{i+1} - c_i)*dI^2 / 3
        //                                     = (p_{i+1} - p_i) - dI^2*( c_i + (c_{i+1} - c_i)/3 )
        //                                     = (p_{i+1} - p_i) - dI^2*(3*c_i + c_{i+1} - c_i)/3
        //                                     = (p_{i+1} - p_i) - dI^2*(2*c_i + c_{i+1})/3
        //       divide by dI,             b_i = (p_{i+1} - p_i)/dI - dI*(2*c_i + c_{i+1})/3
        //
        // b_i is taken from right-end interpolation rather than the C1 condition,
        // b_i = b_{i-1} + 2*c_{i-1}*dI_{i-1} + 3*d_{i-1}*dI_{i-1}^2.
        // C1 cond is a recurrence: sequential, needs a seed b_0, and accumulates
        // rounding error along the chain
        for (std::size_t i = 0; i < m; ++i) {
            const cadm::cadf dI = d[i];
            const cadm::Vec3 ai = p[i];
            const cadm::Vec3 ci = c[i];
            const cadm::Vec3 di = (c[i + 1] - ci) / (3.0f * dI);
            const cadm::Vec3 bi = (p[i + 1] - p[i]) / dI - dI * (2.0f * ci + c[i + 1]) / 3.0f;

            out[3 * i + 0] = ai;
            out[3 * i + 1] = ai + bi * (dI / 3.0f);
            out[3 * i + 2] = ai + bi * (2.0f * dI / 3.0f) + ci * (dI * dI / 3.0f);
            out[3 * i + 3] = ai + bi * dI + ci * (dI * dI) + di * (dI * dI * dI);
        }
    }

    std::vector<cadm::Vec3> solveC2Coeffs(
        const std::vector<cadm::Vec3> &p,
        const std::vector<cadm::cadf> &d
    ) {
        const std::size_t n = p.size();
        std::vector<cadm::Vec3> c(n);
        if (n <= 2) {
            return c;
        }

        const auto alpha = [&](const int i) {
            return d[i - 1] / (d[i - 1] + d[i]);
        };
        const auto beta = [&](const int i) {
            return d[i] / (d[i - 1] + d[i]);
        };
        const auto r = [&](const int i) {
            return 3.0f * ((p[i + 1] - p[i]) / d[i] - (p[i] - p[i - 1]) / d[i - 1])
                / (d[i - 1] + d[i]);
        };

        // A unique solution is obtained by adding two end conditions: flatness at both ends
        // (natural spline, c_0 = c_n = 0)
        //
        //   | 2        beta_1                                                 | | c_1     |   | r_1     |
        //   | alpha_2  2        beta_2                                        | | c_2     |   | r_2     |
        //   |          ...      ...      ...                                  | | ...     | = | ...     |
        //   |                   alpha_i  2            beta_i                  | | c_i     |   | r_i     |
        //   |                            ...          ...          ...        | | ...     |   | ...     |
        //   |                            alpha_{n-2}  2            beta_{n-2} | | c_{n-2} |   | r_{n-2} |
        //   |                                         alpha_{n-1}  2          | | c_{n-1} |   | r_{n-1} |
        //

        // After the forward pass
        //
        //   | 1  beta'_1                                                      | | c_1     |   | r'_1     |
        //   |    1        beta'_2                                             | | c_2     |   | r'_2     |
        //   |             ...          ...                                    | | ...     | = | ...      |
        //   |                          1            beta'_i                   | | c_i     |   | r'_i     |
        //   |                                       ...          ...          | | ...     |   | ...      |
        //   |                                       1            beta'_{n-2}  | | c_{n-2} |   | r'_{n-2} |
        //   |                                                    1            | | c_{n-1} |   | r'_{n-1} |

        // entries 0 and n-1 stay unused
        std::vector<cadm::cadf> betaPrime(n);
        std::vector<cadm::Vec3> rPrime(n);
        const int last = static_cast<int>(n) - 2; // last interior row

        // forward pass over interior
        betaPrime[1] = beta(1) / 2.0f;
        rPrime[1] = r(1) / 2.0f;
        for (int i = 2; i <= last; ++i) {
            // subtract alpha_i * (row_{i-1}) (kill the sub-diagonal, leaving diagonal = pivot)
            const cadm::cadf pivot = 2.0f - alpha(i) * betaPrime[i - 1];
            // divide the row by pivot (the diagonal becomes 1)
            betaPrime[i] = beta(i) / pivot;
            rPrime[i] = (r(i) - alpha(i) * rPrime[i - 1]) / pivot;
        }

        // back-substitution
        c[last] = rPrime[last];
        for (int i = last - 1; i >= 1; --i) {
            c[i] = rPrime[i] - betaPrime[i] * c[i + 1];
        }
        return c;
    }

    std::vector<cadm::cadf> knotIntervals(const std::vector<cadm::Vec3> &p) {
        std::vector<cadm::cadf> d(p.size() - 1);
        for (size_t i = 0; i < d.size(); ++i) {
            d[i] = (p[i + 1] - p[i]).length();
        }
        return d;
    }

    std::vector<cadm::Vec3> dedup(const std::span<const cadm::Vec3> points) {
        std::vector<cadm::Vec3> p;
        for (const auto &pt : points) {
            if (p.empty() || (pt - p.back()).lengthSquared() > gc_minChordSq) {
                p.push_back(pt);
            }
        }
        return p;
    }
}
