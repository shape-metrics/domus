cmake_minimum_required(VERSION 3.16)
project(DOMUS VERSION 1.0 LANGUAGES CXX)

# Enable compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common source files
set(COMMON_SRCS
    src/sat/glucose.cpp
    src/sat/cnf_builder.cpp
    src/orthogonal/shape/shape.cpp
    src/orthogonal/shape/shape_builder.cpp
    src/orthogonal/shape/variables_handler.cpp
    src/orthogonal/shape/clauses_functions.cpp
    src/orthogonal/area_compacter.cpp
    src/orthogonal/equivalence_classes.cpp
    src/drawing/polygon.cpp
    src/core/graph/graphs_algorithms.cpp
    src/core/graph/graph.cpp
    src/core/graph/cycle.cpp
    src/core/graph/attributes.cpp
    src/core/graph/file_loader.cpp
    src/core/tree/tree.cpp
    src/core/tree/tree_algorithms.cpp
    src/drawing/svg_drawer.cpp
    src/orthogonal/drawing_builder.cpp
    src/orthogonal/drawing_stats.cpp
    src/config/config.cpp
    src/core/graph/generators.cpp
    src/orthogonal/file_loader.cpp
    src/core/utils.cpp
    src/core/csv.cpp
)

# Core library
add_library(core STATIC ${COMMON_SRCS})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Optimization flags for core
target_compile_options(core PRIVATE -O3 -march=native)

# Executables
add_executable(main src/main.cpp)
add_executable(stats src/stats.cpp)
add_executable(gen src/gen.cpp)

# Link core library
target_link_libraries(main PRIVATE core)
target_link_libraries(stats PRIVATE core)
target_link_libraries(gen PRIVATE core)

add_subdirectory(OGDF)
add_executable(stats_ogdf
    src/stats_ogdf.cpp
    src/baseline-ogdf/drawer.cpp
)
target_link_libraries(stats_ogdf PRIVATE core OGDF)