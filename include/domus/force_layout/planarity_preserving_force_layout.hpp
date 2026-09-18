#pragma once

#include <cstddef>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

namespace domus::force_layout {

struct ForceLayoutConfig {
    size_t iterations = 100;
    double initial_step = 0.05;
    double cooling_factor = 0.95;
    double k = 0.0; // 0.0 means auto-computed based on bounding box and node count
    double repulsion_strength = 1.0;
    double attraction_strength = 1.0;
    double min_triangle_area = 1e-7;
};

/**
 * @brief Relaxes node positions using a topology-preserving force-directed layout.
 *
 * Given an initial crossing-free embedding (e.g. from Tutte), this function moves
 * internal nodes to relieve clustering while strictly preserving planarity (no edge crossings)
 * by guarding the orientation of all incident triangles around moving nodes.
 *
 * @param graph The underlying graph.
 * @param attributes Contains the 2D positions of nodes (updated in place).
 * @param embedding The combinatorial planar embedding with rotation schemes.
 * @param fixed_nodes Nodes that must not be moved (e.g. boundary vertices).
 * @param config Configuration parameters for the force simulation.
 */
void planarity_preserving_force_layout(
    const graph::Graph& graph,
    graph::Attributes& attributes,
    const graph::Embedding& embedding,
    const graph::utilities::NodesContainer& fixed_nodes = {},
    const ForceLayoutConfig& config = {}
);

} // namespace domus::force_layout
