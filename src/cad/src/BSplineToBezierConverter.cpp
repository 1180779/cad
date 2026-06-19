//
// Created on 5/27/26.
//
#include "BSplineToBezierConverter.hpp"

void bsplineToBezier::detail::insertKnot(
    std::vector<cadm::vec3> &pts,
    std::vector<float> &knots,
    const float tHat,
    const int p
) {
    int k = -1;
    for (int i = 0; i < static_cast<int>(knots.size()) - 1; ++i) {
        if (knots[i] <= tHat && tHat < knots[i + 1]) {
            k = i;
        }
    }
    if (k < 0) {
        return;
    }

    const int n = static_cast<int>(pts.size()) - 1;

    std::vector<cadm::vec3> newPts(pts.size() + 1);
    for (int i = 0; i <= k - p; ++i) {
        newPts[i] = pts[i];
    }
    for (int i = k - p + 1; i <= k; ++i) {
        const float denom = knots[i + p] - knots[i];
        const float alpha = denom > 0.f
                                ? (tHat - knots[i]) / denom
                                : 1.f;
        newPts[i] = pts[i] * alpha + pts[i - 1] * (1.f - alpha);
    }
    for (int i = k + 1; i <= n + 1; ++i) {
        newPts[i] = pts[i - 1];
    }

    std::vector<float> newKnots(knots.size() + 1);
    for (int i = 0; i <= k; ++i) {
        newKnots[i] = knots[i];
    }
    newKnots[k + 1] = tHat;
    for (int i = k + 1; i < static_cast<int>(knots.size()); ++i) {
        newKnots[i + 1] = knots[i];
    }

    pts = std::move(newPts);
    knots = std::move(newKnots);
}

void bsplineToBezier::uniformSegment(
    const cadm::vec3 d0,
    const cadm::vec3 d1,
    const cadm::vec3 d2,
    const cadm::vec3 d3,
    std::span<cadm::vec3, 4> &view
) {
    view[0] = (d0 + d1 * 4.0f + d2) * (1.0f / 6.0f);
    view[1] = (d1 * 4.0f + d2 * 2.0f) * (1.0f / 6.0f);
    view[2] = (d1 * 2.0f + d2 * 4.0f) * (1.0f / 6.0f);
    view[3] = (d1 + d2 * 4.0f + d3) * (1.0f / 6.0f);
}

void bsplineToBezier::chordLength(
    const std::span<const PointHandle> handles,
    const PointRegistry &registry,
    std::vector<cadm::vec3> &out
) {
    const int n = static_cast<int>(handles.size());
    if (n < 4) {
        out.clear();
        return;
    }
    const int segments = n - 3;

    std::vector<cadm::vec3> pts(n);
    for (int i = 0; i < n; ++i) {
        pts[i] = registry.getPosition(handles[i]);
    }

    // chord lengths l[i] = ||d[i+1] - d[i]||
    std::vector<float> l(n - 1);
    for (int i = 0; i < n - 1; ++i) {
        const cadm::vec3 diff = pts[i + 1] - pts[i];
        l[i] = diff.length();
        if (constexpr float kMinChord = cadm::eps;
            l[i] < kMinChord) {
            l[i] = kMinChord;
        }
    }

    // build knot vector (n + 4 knots).
    // Active range: knots[3]...knots[n], each interior span = corresponding chord length.
    // Boundary knots extend beyond the active range with the nearest chord spacing,
    // so Boehm's alpha formula has valid denominators at the first and last segments.
    std::vector<float> knots(n + 4);
    knots[3] = 0.f;
    for (int i = 4; i <= n; ++i) {
        knots[i] = knots[i - 1] + l[i - 4];
    }
    knots[2] = knots[3] - l[0];
    knots[1] = knots[2] - l[0];
    knots[0] = knots[1] - l[0];
    knots[n + 1] = knots[n] + l[n - 2];
    knots[n + 2] = knots[n + 1] + l[n - 2];
    knots[n + 3] = knots[n + 2] + l[n - 2];

    // insert each interior knot twice to raise multiplicity from 1 to 3
    for (const std::vector interior(knots.begin() + 4, knots.begin() + n);
         const float tk : interior) {
        detail::insertKnot(pts, knots, tk, 3);
        detail::insertKnot(pts, knots, tk, 3);
    }

    // pts now has 3n-8 entries. Each interior knot is at multiplicity 3, so the polygon
    // is piecewise Bezier: pts[3k...3k+3] are the 4 Bernstein points of the segment k.
    // Expand into the flat 4-per-segment layout used by the component
    out.resize(static_cast<size_t>(segments) * 4);
    for (int k = 0; k < segments; ++k) {
        out[4 * k + 0] = pts[3 * k + 0];
        out[4 * k + 1] = pts[3 * k + 1];
        out[4 * k + 2] = pts[3 * k + 2];
        out[4 * k + 3] = pts[3 * k + 3];
    }
}

void bsplineToBezier::uniform(
    const std::span<const PointHandle> handles,
    const PointRegistry &registry,
    std::vector<cadm::vec3> &out
) {
    const int n = static_cast<int>(handles.size());
    if (n < 4) {
        out.clear();
        return;
    }
    const int segments = n - 3;
    out.resize(static_cast<size_t>(segments) * 4);
    for (int i = 0; i < segments; ++i) {
        std::span<cadm::vec3, 4> view(std::span{out}.subspan(4 * i, 4));
        bsplineToBezier::uniformSegment(
            registry.getPosition(handles[i]),
            registry.getPosition(handles[i + 1]),
            registry.getPosition(handles[i + 2]),
            registry.getPosition(handles[i + 3]),
            view
        );
    }
}

void bsplineToBezier::convert(
    const ParametrizationMode mode,
    const std::span<const PointHandle> handles,
    const PointRegistry &registry,
    std::vector<cadm::vec3> &out
) {
    if (mode == ParametrizationMode::chordLength) {
        chordLength(handles, registry, out);
        return;
    }
    uniform(handles, registry, out);
}
