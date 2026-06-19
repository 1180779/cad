//
// Created on 5/6/26.
//

#ifndef CAD_INEWPOINTSTARGET_HPP
#define CAD_INEWPOINTSTARGET_HPP

#include "Entity.hpp"
#include "PointRegistry.hpp"

/// Base class for @ref INewPointsTargetComponent for getting component purposes
class INewPointsTargetBase {
public:
    virtual ~INewPointsTargetBase() = default;

    virtual void addControlPoint(PointHandle h) = 0;

    virtual void removeControlPoint(PointHandle h) = 0;
};

/// Component that can be a new points target
/// (i.e., set to auto add newly added points to it)
/// @note uses CRTP pattern to enforce only usage by components
/// @note use the @ref INewPointsTargetBase to retrieve the component
template <typename Derived>
class INewPointsTargetComponent : public INewPointsTargetBase {
public:
    ~INewPointsTargetComponent() override {
        static_assert(
            std::is_base_of_v<Component, Derived>,
            "INewPointsTargetComponent must only be inherited by a class that also inherits Component"
        );
    }
};

#endif //CAD_INEWPOINTSTARGET_HPP
