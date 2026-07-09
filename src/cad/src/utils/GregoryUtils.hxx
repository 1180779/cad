//
// Created by Radosław Głasek on 09.07.2026
//

#ifndef CAD_GREGORYUTILS_HXX
#define CAD_GREGORYUTILS_HXX

#include <algorithm>
#include <array>
#include <span>
#include <vector>

#include <cad_math/Vec3.hpp>
#include "BezierUtils.hpp"

namespace gregory {
    using bezierUtils::Curve4;

    /// @brief One hole edge resolved to positions: the boundary row of the C0
    /// patch and the row adjacent to it, both oriented along the hole cycle
    struct EdgeData {
        Curve4 boundary{};
        Curve4 inner{};
    };

    /// @brief 20 control points of a single Gregory net
    ///
    /// Let <tt>P</tt> be the central point where all Gregory patches of the
    /// hole meet. Each hole edge <tt>i</tt> gets an internal curve running from
    /// the middle of that edge to the center:
    /// - <tt>P_3i</tt> -- split point of the surface boundary curve (t = 1/2)
    /// - <tt>P_2i</tt> -- midpoint of the reflected (C1-continuation) row
    /// - <tt>P_1i</tt> -- <tt>(2 Q_i + P) / 3</tt>
    /// - <tt>P</tt>    -- <tt>avg(Q_i)</tt>, where the auxiliary
    ///   <tt>Q_i = (3 P_2i - P_3i) / 2</tt>
    ///
    /// Patch <tt>i</tt> spans internal curves <tt>i</tt> and <tt>i+1</tt>. Its
    /// four corners:
    /// - <tt>A = P_3i</tt> (middle of hole edge i)
    /// - <tt>B</tt> — the sole corner lying on the hole boundary: the original
    ///   hole corner shared by edges i and i+1
    /// - <tt>C = P_3(i+1)</tt> (middle of hole edge i+1)
    /// - <tt>D = P</tt> (the center, shared by all patches)
    /// 
    /// The internal-curve points <tt>P_3j, P_2j, P_1j</tt> are shared between
    /// the two patches adjacent to internal curve j. u runs A->B, v runs A->D.
    ///
    /// Layout:
    /// - [ 0...11]: edges A -> B -> C -> D -> A, i.e.,
    ///    - [ 0... 3]:   row v=0: A...B (second half of hole edge i)
    ///    - [ 4... 5]:   column u=1 interior ring points, i.e. B...C (first half of
    ///                   hole edge i+1) without its endpoints
    ///    - [ 6... 9]:   row v=1 reversed: [6]=C=P_3(i+1), [7]=P_2(i+1), [8]=P_1(i+1), [9]=D=P
    ///    - [10...11]:   column u=0 interior ring points, continuing the ring from D
    ///                   back toward A: [10]=P_1i, [11]=P_2i
    /// - [12..19]: interior (twist) points, two per corner:
    ///    - [12]: f00u  (corner A)
    ///    - [13]: f00v
    ///    - [14]: f10u  (corner B) 
    ///    - [15]: f10v
    ///    - [16]: f11u  (corner C)  
    ///    - [17]: f11v
    ///    - [18]: f01u  (corner D) 
    ///    - [19]: f01v
    ///
    struct Net {
        enum Corner {
            a = 0,
            b = 1,
            c = 2,
            d = 3
        };

        std::array<cadm::Vec3, 20> pts{};

        void fillEdges(const Curve4 &b, const Curve4 &bP1, const Curve4 &inP1, const Curve4 &in);

        void fillInternalCorner(Corner corner, const cadm::Vec3 &fu, const cadm::Vec3 &fv);

    private:
        void fillB(const Curve4 &b);

        void fillBp1(const Curve4 &bP1);

        void fillInP1(const Curve4 &inP1);

        void fillIn(const Curve4 &in);
    };

    /// @brief Build the Gregory nets filling a closed hole of n >= 3 edges
    [[nodiscard]] std::vector<Net> fillHole(std::span<const EdgeData> edges);

    /// @brief Fixed-size facade over the runtime-length fillHole
    /// @see <tt>fillHole(std::span<const EdgeData>)</tt>
    template <std::size_t N> requires (N >= 3)
    [[nodiscard]] std::array<Net, N> fillHole(const std::array<EdgeData, N> &edges) {
        std::array<Net, N> out{};
        std::ranges::copy(fillHole(std::span<const EdgeData>{edges}), out.begin());
        return out;
    }
}

#endif //CAD_GREGORYUTILS_HXX
