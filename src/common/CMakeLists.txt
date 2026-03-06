add_library(common STATIC
        include/common/DoubleSlider.h
        src/DoubleSlider.cpp
)

set_target_properties(common PROPERTIES AUTOMOC ON)

target_include_directories(common
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(common PUBLIC cxx_std_20)

target_link_libraries(common
        PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)