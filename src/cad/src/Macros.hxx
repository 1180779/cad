//
// Created by Radosław Głasek on 10.07.2026
//

#ifndef CAD_MACROS_HXX
#define CAD_MACROS_HXX

#define ENUM_ELEMENT(element) element,
#define ENUM_ELEMENT_NAME(element) #element,

/// @brief Generates an enum class, a constexpr name table and toString function
/// 
/// Usage:
///   #define PLANE_ELEMENTS(X) X(xy) X(xz) X(yz)
///   DECLARE_ENUM_WITH_TO_STRING(Plane, PLANE_ELEMENTS)
#define DECLARE_ENUM_WITH_TO_STRING(EnumName, ELEMENTS) \
enum class EnumName { ELEMENTS(ENUM_ELEMENT) }; \
inline constexpr const char *EnumName##Names[] = { ELEMENTS(ENUM_ELEMENT_NAME) }; \
inline constexpr std::size_t EnumName##Count = std::size(EnumName##Names); \
constexpr const char *toString(const EnumName value) { \
const auto i = static_cast<std::size_t>(value); \
return i < EnumName##Count ? EnumName##Names[i] : "?"; \
}

#endif //CAD_MACROS_HXX
