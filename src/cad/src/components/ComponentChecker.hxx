//
// Created by Radosław Głasek on 10.07.2026
//

#ifndef CAD_COMPONENTCHECKER_HXX
#define CAD_COMPONENTCHECKER_HXX

#include <memory>
#include <vector>
#include "Entity.hpp"

class IComponentChecker {
public:
    virtual ~IComponentChecker() = default;

    virtual bool checkForComponent(Entity *e) = 0;
};

template <typename T>
class ComponentChecker final : public IComponentChecker {
public:
    bool checkForComponent(Entity *e) override {
        return e->hasComponent<T>();
    }
};

using ComponentFilters = std::vector<std::unique_ptr<IComponentChecker>>;

#endif //CAD_COMPONENTCHECKER_HXX
