//
// Created by Radosław Głasek on 08.07.2026
//

// ReSharper disable CppNonExplicitConvertingConstructor
#ifndef CAD_SINGLEPATCHVIEW_HXX
#define CAD_SINGLEPATCHVIEW_HXX

#include <array>

#include <cad_math/Vec4.hpp>
#include "PointRegistry.hpp"

/// @brief View into a single bicubic patch
class SinglePatchView final {
public:
    using Vec4H = cadm::Vec<PointHandle, 4>;
    using Vec4P = cadm::Vec<cadm::Vec3, 4>;
    using VecVec4H = cadm::Vec<Vec4H, 4>;
    using VecVec4P = cadm::Vec<Vec4P, 4>;

    explicit SinglePatchView(const VecVec4H &rowsOfColumns)
    : m_rowsOfColumns(rowsOfColumns) {}

    /// @brief Implicit inverse of <tt>handles()</tt>: 16 control points,
    /// row-major
    SinglePatchView(const std::array<PointHandle, 16> &handles);

    /// @brief All 16 control points, row-major
    [[nodiscard]] const VecVec4H& grid() const {
        return m_rowsOfColumns;
    }

    /// @brief The 16 control points flattened row-major
    [[nodiscard]] std::array<PointHandle, 16> handles() const;

    /// @brief The four corners: top: left, top right, bottom right, bottom left
    [[nodiscard]] Vec4H corners() const;

    [[nodiscard]] Vec4P cornersPos(const PointRegistry *registry) const;

    /// @brief The four boundary edges: left, top, right, bottom
    [[nodiscard]] VecVec4H edges() const;

    [[nodiscard]] VecVec4P edgesPos(const PointRegistry *registry) const;

private:
    Vec4P static hToP(Vec4H h, const PointRegistry *registry);

    VecVec4H m_rowsOfColumns{};
};

inline SinglePatchView::Vec4P SinglePatchView::hToP(const Vec4H h, const PointRegistry *registry) {
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

inline SinglePatchView::Vec4H SinglePatchView::corners() const {
    const auto firstRow = m_rowsOfColumns[0];
    const auto lastRow = m_rowsOfColumns[3];
    return {firstRow[0], firstRow[3], lastRow[3], lastRow[0]};
}

inline SinglePatchView::Vec4P SinglePatchView::cornersPos(const PointRegistry *registry) const {
    return hToP(corners(), registry);
}

inline SinglePatchView::VecVec4H SinglePatchView::edges() const {
    const Vec4H left{m_rowsOfColumns[0][0], m_rowsOfColumns[1][0], m_rowsOfColumns[2][0], m_rowsOfColumns[3][0]};
    const Vec4H top{m_rowsOfColumns[0][0], m_rowsOfColumns[0][1], m_rowsOfColumns[0][2], m_rowsOfColumns[0][3]};
    const Vec4H right{m_rowsOfColumns[0][3], m_rowsOfColumns[1][3], m_rowsOfColumns[2][3], m_rowsOfColumns[3][3]};
    const Vec4H bottom{m_rowsOfColumns[3][0], m_rowsOfColumns[3][1], m_rowsOfColumns[3][2], m_rowsOfColumns[3][3]};
    return {left, top, right, bottom};
}

inline SinglePatchView::VecVec4P SinglePatchView::edgesPos(const PointRegistry *registry) const {
    const auto e = edges();
    return {
        hToP(e[0], registry),
        hToP(e[1], registry),
        hToP(e[2], registry),
        hToP(e[3], registry)
    };
}

#endif //CAD_SINGLEPATCHVIEW_HXX
