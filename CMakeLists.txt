cmake_minimum_required(VERSION 3.16)

project(DOMUS
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#====================================================
# Options
#====================================================

option(DOMUS_BUILD_EXECUTABLES "Build DOMUS executables" ON)

#====================================================
# Detect Emscripten
#====================================================

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS ">>> Building DOMUS for WebAssembly")
    set(BUILDING_WASM TRUE)
else()
    set(BUILDING_WASM FALSE)
endif()

#====================================================
# SAT Solvers
#====================================================

# ---- Kissat --------------------------------------------------
file(GLOB KISSAT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sat/kissat/src/*.c
)

add_library(kissat STATIC ${KISSAT_SRC})
target_include_directories(kissat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sat/kissat/src
)
target_compile_definitions(kissat PRIVATE NDEBUG QUIET)

# ---- Glucose -------------------------------------------------
find_package(Threads REQUIRED) # necessary?

file(GLOB GLUCOSE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sat/glucose/src/*.cc
)

add_library(glucose STATIC ${GLUCOSE_SRC})
target_include_directories(glucose PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sat/glucose/src
)

#====================================================
# Core library
#====================================================

set(COMMON_SRCS
    src/sat/kissat.cpp
    src/sat/glucose.cpp
    src/sat/sat.cpp
    src/sat/cnf.cpp
    src/orthogonal/shape/shape.cpp
    src/orthogonal/shape/shape_builder.cpp
    src/orthogonal/shape/variables_handler.cpp
    src/orthogonal/shape/clauses_functions.cpp
    src/orthogonal/area_compacter.cpp
    src/orthogonal/equivalence_classes.cpp
    src/orthogonal/drawing.cpp
    src/drawing/polygon.cpp
    src/core/graph/graphs_algorithms.cpp
    src/core/graph/graph.cpp
    src/core/graph/cycle.cpp
    src/core/graph/attributes.cpp
    src/core/graph/file_loader.cpp
    src/core/tree/tree.cpp
    src/core/tree/tree_algorithms.cpp
    src/core/graph/segment.cpp
    src/drawing/svg_drawer.cpp
    src/orthogonal/drawing_builder.cpp
    src/orthogonal/drawing_stats.cpp
    src/config/config.cpp
    src/core/graph/generators.cpp
    src/core/utils.cpp
    src/core/csv.cpp
    src/planarity/auslander_parter.cpp
    src/planarity/embedding.cpp
    src/planarity/interlacement.cpp
    src/core/graph/palm_tree.cpp
)

add_library(core ${COMMON_SRCS})
add_library(DOMUS::core ALIAS core)

target_compile_features(core PUBLIC cxx_std_23)

target_include_directories(core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(core
    PRIVATE kissat glucose Threads::Threads
)

#====================================================
# Warning flags
#====================================================

function(apply_warnings target)
    if (BUILDING_WASM)
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -fexceptions
        )
    else()
        target_compile_options(${target} PRIVATE
            Enable when desired
            -Wall -Wextra -Wpedantic
            -Wconversion -Wsign-conversion
            -Wshadow -Wold-style-cast
            -Woverloaded-virtual
            -Wnon-virtual-dtor
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
        )
    endif()
endfunction()

apply_warnings(core)

#====================================================
# Installation
#====================================================

install(TARGETS core kissat glucose
    EXPORT DomusTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT DomusTargets
    FILE DomusTargets.cmake
    NAMESPACE DOMUS::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Domus
)

#====================================================
# Executables
#====================================================

if (DOMUS_BUILD_EXECUTABLES)
    if (BUILDING_WASM)
        add_executable(domus src/domus-web.cpp)
        target_link_libraries(domus PRIVATE DOMUS::core)
        apply_warnings(domus)

        target_link_options(domus PRIVATE
            "-sEXPORTED_FUNCTIONS=['_compute_orthogonal_drawing']"
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','UTF8ToString']"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sWASM=1"
            "--preload-file" "${CMAKE_CURRENT_SOURCE_DIR}/example-graphs@/example-graphs"
        )
    else()
        add_executable(domus src/domus.cpp)
        add_executable(gen src/gen.cpp)

        foreach(target domus gen)
            target_link_libraries(${target} PRIVATE DOMUS::core)
            apply_warnings(${target})
        endforeach()
    endif()
endif()
