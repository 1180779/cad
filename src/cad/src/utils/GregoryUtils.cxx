//
// Created by Radosław Głasek on 09.07.2026
//

#include "GregoryUtils.hxx"

#include <stdexcept>

namespace gregory {
    using bezierUtils::lerp;
    using bezierUtils::splitInHalf;

    namespace {
        /// @brief Reflection of @p inner across @p boundary
        Curve4 reflected(const Curve4 &boundary, const Curve4 &inner) {
            return boundary * 2.0f - inner;
        }
    }

    void Net::fillEdges(const Curve4 &b, const Curve4 &bP1, const Curve4 &inP1, const Curve4 &in) {
        fillB(b);
        fillBp1(bP1);
        fillInP1(inP1);
        fillIn(in);
    }

    void Net::fillInternalCorner(const Corner corner, const cadm::Vec3 &fu, const cadm::Vec3 &fv) {
        const auto idx = 12 + 2 * static_cast<int>(corner);
        pts[idx] = fu;
        pts[idx + 1] = fv;
    }

    void Net::fillB(const Curve4 &b) {
        pts[0] = b[0];
        pts[1] = b[1];
        pts[2] = b[2];
        pts[3] = b[3];
    }

    void Net::fillBp1(const Curve4 &bP1) {
        pts[4] = bP1[1];
        pts[5] = bP1[2];
        pts[6] = bP1[3];
    }

    void Net::fillInP1(const Curve4 &inP1) {
        pts[7] = inP1[1];
        pts[8] = inP1[2];
        pts[9] = inP1[3];
    }

    void Net::fillIn(const Curve4 &in) {
        pts[10] = in[2];
        pts[11] = in[1];
    }

    std::vector<Net> fillHole(const std::span<const EdgeData> edges) {
        const std::size_t n = edges.size();

        // prefixes/suffixes:
        // b  = boundary
        // r  = reflected
        // in = interior
        // p1/P1 = plus one

        std::vector<std::array<Curve4, 2>> bHalf(n);
        std::vector<std::array<Curve4, 2>> rHalf(n);
        std::vector<cadm::Vec3> p3(n);
        std::vector<cadm::Vec3> p2(n);
        std::vector<cadm::Vec3> q(n);
        cadm::Vec3 center{};
        for (std::size_t i = 0; i < n; ++i) {
            bHalf[i] = splitInHalf(edges[i].boundary);
            rHalf[i] = splitInHalf(reflected(edges[i].boundary, edges[i].inner));
            p3[i] = bHalf[i][0][3];
            p2[i] = rHalf[i][0][3];
            q[i] = (cadm::cadf{3.0} * p2[i] - p3[i]) * cadm::cadf{0.5};
            center += q[i];
        }
        center *= cadm::cadf{1} / static_cast<cadm::cadf>(n);
        std::vector<cadm::Vec3> p1(n);
        for (std::size_t i = 0; i < n; ++i) {
            p1[i] = (cadm::cadf{2.0} * q[i] + center) * cadm::cadf{1.0 / 3.0};
        }

        const auto internalCurve = [&](const std::size_t i) {
            return Curve4{p3[i], p2[i], p1[i], center};
        };

        std::vector<Net> quads(n);
        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t ip1 = (i + 1) % n;
            const Curve4 &b = bHalf[i][1]; // from A to B
            const Curve4 &bP1 = bHalf[ip1][0]; // from B to C
            const Curve4 in = internalCurve(i); // from A to P
            const Curve4 inP1 = internalCurve(ip1); // from C to P

            const Curve4 &r = rHalf[i][1];
            const Curve4 &rP1 = rHalf[ip1][0];

            const cadm::Vec3 duAtAv0 = (b[1] - b[0]) * 3.0;
            const cadm::Vec3 duAtDv1 = (inP1[2] - inP1[3]) * 3.0;
            const cadm::Vec3 minusDvAtDu0 = (in[2] - in[3]) * 3.0;
            const cadm::Vec3 minusDvAtDu1 = (bP1[2] - bP1[3]) * 3.0;
            constexpr double third = 1.0 / 3.0;
            const auto field = [](const cadm::Vec3 &g0, const cadm::Vec3 &g1, const cadm::cadf t) {
                return lerp(g0, g1, t) * third;
            };

            auto &net = quads[i];
            net.fillEdges(b, bP1, inP1, in);
            net.fillInternalCorner(Net::a, r[1], in[1] + field(duAtAv0, duAtDv1, third));
            net.fillInternalCorner(Net::b, r[2], rP1[1]);
            net.fillInternalCorner(Net::c, inP1[1] + field(minusDvAtDu0, minusDvAtDu1, 2.0 * third), rP1[2]);
            net.fillInternalCorner(
                Net::d,
                inP1[2] + field(minusDvAtDu0, minusDvAtDu1, third),
                in[2] + field(duAtAv0, duAtDv1, 2.0 * third)
            );
        }
        return quads;
    }
}
