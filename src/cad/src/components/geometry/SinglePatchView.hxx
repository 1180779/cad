//
// Created by Radosław Głasek on 08.07.2026
//

// ReSharper disable CppNonExplicitConvertingConstructor
#ifndef CAD_SINGLEPATCHVIEW_HXX
#define CAD_SINGLEPATCHVIEW_HXX

#include <array>

#include <cad_math/Vec4.hpp>
#include "PointRegistry.hpp"
#include "utils/BezierUtils.hpp"

/// @brief View into a single bicubic patch
class SinglePatchView final {
public:
    explicit SinglePatchView(const bezierUtils::HandleGrid4x4 &rowsOfColumns)
    : m_rowsOfColumns(rowsOfColumns) {}

    /// @brief Implicit inverse of <tt>handles()</tt>: 16 control points,
    /// row-major
    SinglePatchView(const std::array<PointHandle, 16> &handles);

    /// @brief All 16 control points, row-major
    [[nodiscard]] const bezierUtils::HandleGrid4x4& grid() const {
        return m_rowsOfColumns;
    }

    /// @brief The 16 control points flattened row-major
    [[nodiscard]] std::array<PointHandle, 16> handles() const;

    /// @brief The four corners: top: left, top right, bottom right, bottom left
    [[nodiscard]] bezierUtils::HandleCurve4 corners() const;

    [[nodiscard]] bezierUtils::Curve4 cornersPos(const PointRegistry *registry) const;

    /// @brief The four boundary edges: left, top, right, bottom
    [[nodiscard]] bezierUtils::HandleGrid4x4 edges() const;

    [[nodiscard]] bezierUtils::Grid4x4 edgesPos(const PointRegistry *registry) const;

    /// @brief The rows adjacent to the four boundary edges, in the same order
    /// as <tt>edges()</tt> (left, top, right, bottom)
    [[nodiscard]] bezierUtils::HandleGrid4x4 innerEdges() const;

private:
    static bezierUtils::Curve4 hToP(bezierUtils::HandleCurve4 h, const PointRegistry *registry);

    bezierUtils::HandleGrid4x4 m_rowsOfColumns{};
};

inline bezierUtils::Curve4 SinglePatchView::hToP(const bezierUtils::HandleCurve4 h, const PointRegistry *registry) {
    return {
        registry->getPosition(h[0]),
        registry->getPosition(h[1]),
        registry->getPosition(h[2]),
        registry->getPosition(h[3]),
    };
}

inline SinglePatchView::SinglePatchView(const std::array<PointHandle, 16> &handles) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m_rowsOfColumns[i][j] = handles[i * 4 + j];
        }
    }
}

inline std::array<PointHandle, 16> SinglePatchView::handles() const {
    std::array<PointHandle, 16> out{};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            out[i * 4 + j] = m_rowsOfColumns[i][j];
        }
    }
    return out;
}

inline bezierUtils::HandleCurve4 SinglePatchView::corners() const {
    const auto firstRow = m_rowsOfColumns[0];
    const auto lastRow = m_rowsOfColumns[3];
    return {firstRow[0], firstRow[3], lastRow[3], lastRow[0]};
}

inline bezierUtils::Curve4 SinglePatchView::cornersPos(const PointRegistry *registry) const {
    return hToP(corners(), registry);
}

inline bezierUtils::HandleGrid4x4 SinglePatchView::edges() const {
    const bezierUtils::HandleCurve4 left{
        m_rowsOfColumns[0][0],
        m_rowsOfColumns[1][0],
        m_rowsOfColumns[2][0],
        m_rowsOfColumns[3][0]
    };
    const bezierUtils::HandleCurve4 top{
        m_rowsOfColumns[0][0],
        m_rowsOfColumns[0][1],
        m_rowsOfColumns[0][2],
        m_rowsOfColumns[0][3]
    };
    const bezierUtils::HandleCurve4 right{
        m_rowsOfColumns[0][3],
        m_rowsOfColumns[1][3],
        m_rowsOfColumns[2][3],
        m_rowsOfColumns[3][3]
    };
    const bezierUtils::HandleCurve4 bottom{
        m_rowsOfColumns[3][0],
        m_rowsOfColumns[3][1],
        m_rowsOfColumns[3][2],
        m_rowsOfColumns[3][3]
    };
    return {left, top, right, bottom};
}

inline bezierUtils::HandleGrid4x4 SinglePatchView::innerEdges() const {
    const bezierUtils::HandleCurve4 left{
        m_rowsOfColumns[0][1],
        m_rowsOfColumns[1][1],
        m_rowsOfColumns[2][1],
        m_rowsOfColumns[3][1]
    };
    const bezierUtils::HandleCurve4 top{
        m_rowsOfColumns[1][0],
        m_rowsOfColumns[1][1],
        m_rowsOfColumns[1][2],
        m_rowsOfColumns[1][3]
    };
    const bezierUtils::HandleCurve4 right{
        m_rowsOfColumns[0][2],
        m_rowsOfColumns[1][2],
        m_rowsOfColumns[2][2],
        m_rowsOfColumns[3][2]
    };
    const bezierUtils::HandleCurve4 bottom{m_rowsOfColumns[2][0], m_rowsOfColumns[2][1], m_rowsOfColumns[2][2], m_rowsOfColumns[2][3]};
    return {left, top, right, bottom};
}

inline bezierUtils::Grid4x4 SinglePatchView::edgesPos(const PointRegistry *registry) const {
    const auto e = edges();
    return {
        hToP(e[0], registry),
        hToP(e[1], registry),
        hToP(e[2], registry),
        hToP(e[3], registry)
    };
}

#endif //CAD_SINGLEPATCHVIEW_HXX
