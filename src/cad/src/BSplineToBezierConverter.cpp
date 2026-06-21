//
// Created on 5/27/26.
//
#include "BSplineToBezierConverter.hpp"

void bsplineToBezier::uniformSegment(
    const cadm::Vec3 d0,
    const cadm::Vec3 d1,
    const cadm::Vec3 d2,
    const cadm::Vec3 d3,
    std::span<cadm::Vec3, 4> view
) {
    const auto prevLegTwoThird = d0 * (1.0f / 3.0f) + d1 * (2.0f / 3.0f);
    const auto legOneThird = d1 * (2.0f / 3.0f) + d2 * (1.0f / 3.0f);
    const auto legTwoThird = d1 * (1.0f / 3.0f) + d2 * (2.0f / 3.0f);
    const auto nextLegOneThird = d2 * (2.0f / 3.0f) + d3 * (1.0f / 3.0f);

    view[0] = prevLegTwoThird * 0.5f + legOneThird * 0.5f;
    view[1] = legOneThird;
    view[2] = legTwoThird;
    view[3] = legTwoThird * 0.5f + nextLegOneThird * 0.5f;
}

void bsplineToBezier::convert(
    const std::span<const PointHandle> handles,
    const PointRegistry &registry,
    std::vector<cadm::Vec3> &out
) {
    const int n = static_cast<int>(handles.size());
    if (n < 4) {
        out.clear();
        return;
    }
    const int segments = n - 3;
    out.resize(static_cast<std::size_t>(segments) * 3 + 1);
    for (int i = 0; i < segments; ++i) {
        uniformSegment(
            registry.getPosition(handles[i]),
            registry.getPosition(handles[i + 1]),
            registry.getPosition(handles[i + 2]),
            registry.getPosition(handles[i + 3]),
            std::span<cadm::Vec3, 4>(
                std::span{out}.subspan(static_cast<std::size_t>(3) * i, 4)
            )
        );
    }
}
