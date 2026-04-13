#include "domus/planarity/tutte.hpp"

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"

#include "domus/core/domus_debug.hpp"

#include <Eigen/Sparse>
#include <Eigen/SparseLU>

namespace domus::planarity {
using namespace domus::graph;

void compute_tutte_layout(const domus::graph::Graph& graph, Attributes& attributes) {
    const long n = static_cast<long>(graph.get_number_of_nodes());
    Eigen::SparseMatrix<double> A(n, n);
    Eigen::VectorXd b_x = Eigen::VectorXd::Zero(n);
    Eigen::VectorXd b_y = Eigen::VectorXd::Zero(n);

    // triplet list to build the sparse matrix efficiently
    std::vector<Eigen::Triplet<double>> triplets;

    for (const size_t i : graph.get_nodes_ids()) {
        const long node_id = static_cast<long>(i);
        if (attributes.has_position(i)) {
            // boundary position is fixed
            // row in matrix: 1.0 * pos_i = boundary_pos
            triplets.emplace_back(i, i, 1.0);
            b_x(node_id) = attributes.get_position_x(i);
            b_y(node_id) = attributes.get_position_y(i);
        } else {
            // interior node: position is average of neighbors
            // row in matrix: pos_i - (1/deg) * sum(pos_neighbors) = 0
            double degree = static_cast<double>(graph.get_degree_of_node(i));
            triplets.emplace_back(i, i, 1.0);

            if (degree > 0) {
                double weight = -1.0 / degree;
                for (size_t neighbor : graph.get_neighbors(i)) {
                    triplets.emplace_back(i, neighbor, weight);
                }
            }
            // b_x(i) and b_y(i) remain 0
        }
    }

    A.setFromTriplets(triplets.begin(), triplets.end());

    // solve the systems
    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    solver.compute(A);

    Eigen::VectorXd res_x = solver.solve(b_x);
    Eigen::VectorXd res_y = solver.solve(b_y);

    for (const size_t i : graph.get_nodes_ids()) {
        const long node_id = static_cast<long>(i);
        if (attributes.has_position(i))
            continue;
        // TODO at the moment positions can only be ints
        const int x = static_cast<int>(res_x(node_id));
        const int y = static_cast<int>(res_y(node_id));
        attributes.set_position(i, x, y);
    }
}

void compute_nodes_positions(const Graph& graph, Attributes& attributes) {
    DOMUS_ASSERT(
        attributes.has_attribute(Attribute::NODES_POSITION),
        "compute_nodes_positions: external border is not initialized"
    );

    DOMUS_ASSERT(
        algorithms::BiconnectedComponents::compute(graph).get_components().size() == 1,
        "compute_nodes_positions: Tutte algorithm's needs the input graph to be triconnected"
    );

    compute_tutte_layout(graph, attributes);
}
} // namespace domus::planarity
