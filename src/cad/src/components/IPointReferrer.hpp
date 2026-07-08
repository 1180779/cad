//
// Created by Radosław Głasek on 08.07.2026
//

#ifndef CAD_IPOINTREFERRER_HPP
#define CAD_IPOINTREFERRER_HPP

#include <vector>

#include "../PointRegistry.hpp"

/// @brief Component that references control points by <tt>PointHandle</tt> and
/// can repoint them
/// @note retrieve via <tt>Entity::getComponent<IPointReferrer></tt>
class IPointReferrer {
public:
    virtual ~IPointReferrer() = default;

    /// @brief Replace every occurrence of @p from with @p to and refresh the
    /// derived geometry. No-op when @p from is not referenced
    virtual void replaceControlPoint(PointHandle from, PointHandle to) = 0;

    /// @brief Current control-point handle list (layout is component-specific)
    [[nodiscard]] virtual std::vector<PointHandle> controlPointHandles() const = 0;

    /// @brief Restore a handle list previously returned by @ref
    /// controlPointHandles, refreshing the derived geometry
    /// @pre for grid-based components the size must match the current list
    virtual void setControlPointHandles(const std::vector<PointHandle> &handles) = 0;
};

#endif //CAD_IPOINTREFERRER_HPP
