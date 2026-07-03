//
// Created by Radosław Głasek on 02.07.2026
//

#ifndef CAD_TOOLS_HXX
#define CAD_TOOLS_HXX

namespace tools {
    /// @brief Helper for exhaustive std::visit
    /// @note From https://en.cppreference.com/cpp/utility/variant/visit2
    template <typename... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };

    /// @brief Explicit deduction guide (not needed as of C++20)
    template <typename... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;
}

#endif //CAD_TOOLS_HXX
