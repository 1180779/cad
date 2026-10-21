add_library(cnc_core STATIC
        src/NcProgram.cxx
        src/Simulator.cxx
        src/ToolpathFit.cxx
)

target_include_directories(cnc_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cnc_core
        PUBLIC
        cad_math::cad_math
)

target_compile_features(cnc_core PUBLIC cxx_std_20)
