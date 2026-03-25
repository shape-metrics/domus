#include "domus/orthogonal/shape/shape_builder.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <random>
#include <string>
#include <utility>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/sat/cnf.hpp"
#include "domus/sat/sat.hpp"

#include "../../core/domus_debug.hpp"
#include "clauses_functions.hpp"
#include "variables_handler.hpp"

namespace domus::orthogonal::shape {
using namespace graph::algorithms;
using namespace sat;
using namespace graph;

Shape result_to_shape(
    const Graph& graph, const std::vector<int>& numbers, VariablesHandler& handler
) {
    for (const int var : numbers) {
        if (var > 0)
            handler.set_variable_value(static_cast<size_t>(var), true);
        else
            handler.set_variable_value(static_cast<size_t>(-var), false);
    }
    Shape shape;
    for (size_t node_id : graph.get_node_ids())
        for (auto [edge_id, neighbor_id] : graph.get_out_edges(node_id)) {
            Direction direction = handler.get_direction_of_edge(edge_id);
            shape.set_direction(edge_id, direction);
        }
    return shape;
}

size_t find_edge_id_to_split(
    const std::vector<std::string>& proof_lines,
    std::mt19937& random_engine,
    const VariablesHandler& handler,
    size_t number_of_variables
) {
    std::vector<int> unit_clauses;
    for (size_t i = proof_lines.size(); i > 0; i--) {
        const std::string& line = proof_lines[i - 1];
        // split line based on " "
        std::vector<int> tokens;
        std::string token;
        for (char c : line) {
            if (c == 'd')
                continue;
            if (c == ' ') {
                if (token.empty())
                    continue;
                tokens.push_back(std::stoi(token));
                token = "";
            } else
                token += c;
        }
        DOMUS_ASSERT(token == "0", "find_edges_to_split: last token should be 0");
        if (tokens.size() == 1) {
            int unit_clause = tokens[0];
            if (static_cast<size_t>(std::abs(unit_clause)) <= number_of_variables)
                unit_clauses.push_back(unit_clause);
        }
    }
    DOMUS_ASSERT(
        !unit_clauses.empty(),
        "find_edges_to_split: no unit clauses found"
    ); // Could not find the edge to remove
    // pick one of the first two unit clauses
    size_t random_index = random_engine() % std::min(unit_clauses.size(), static_cast<size_t>(2));
    size_t variable = static_cast<size_t>(std::abs(unit_clauses[random_index]));
    return handler.get_edge_id_of_variable(variable);
}

std::optional<Shape> build_shape_or_add_corner(
    Graph& graph,
    GraphAttributes& attributes,
    std::vector<Cycle>& cycles,
    std::mt19937& random_engine
);

Shape build_shape(
    Graph& graph, GraphAttributes& attributes, std::vector<Cycle>& cycles, const bool randomize
) {
    const size_t seed = randomize ? std::random_device{}() : 42;
    std::mt19937 random_engine(seed);
    DOMUS_ASSERT(
        [&]() {
            for (const Cycle& cycle : cycles)
                if (!is_cycle_in_graph(graph, cycle))
                    return false;
            return true;
        }(),
        "build_shape: a cycle is not valid"
    );
    std::optional<Shape> shape =
        build_shape_or_add_corner(graph, attributes, cycles, random_engine);
    while (!shape.has_value())
        shape = build_shape_or_add_corner(graph, attributes, cycles, random_engine);
    return std::move(shape.value());
}

void add_corner_inside_edge(
    size_t edge_id, Graph& graph, GraphAttributes& attributes, std::vector<Cycle>& cycles
) {
    graph::Subdivision subdivision = graph.subdivide_edge(edge_id);
    attributes.set_node_color(subdivision.in_between_id, Color::RED);
    for (Cycle& cycle : cycles)
        if (cycle.has_edge_id(edge_id)) {
            graph.add_subdivision_to_cycle(subdivision, cycle);
            DOMUS_ASSERT(
                is_cycle_in_graph(graph, cycle),
                "add_corner_inside_edge: after subdividing cycle is not valid"
            );
        }
}

std::optional<Shape> build_shape_or_add_corner(
    Graph& graph,
    GraphAttributes& attributes,
    std::vector<Cycle>& cycles,
    std::mt19937& random_engine
) {
    VariablesHandler handler(graph);
    cnf::Cnf cnf{};
    // cnf.add_comment("constraints one direction per edge");
    add_constraints_one_direction_per_edge(graph, cnf, handler);
    // cnf.add_comment("constraints nodes");
    add_nodes_constraints(graph, cnf, handler);
    // cnf.add_comment("constraints cycles");
    add_cycles_constraints(graph, cnf, cycles, handler);
    const auto [result, numbers, proof_lines] = launch_glucose(cnf);
    if (result == SatSolverResultType::UNSAT) {
        const size_t edge_id = find_edge_id_to_split(
            proof_lines,
            random_engine,
            handler,
            cnf.get_number_of_variables()
        );
        add_corner_inside_edge(edge_id, graph, attributes, cycles);
        return std::nullopt;
    }
    Shape shape = result_to_shape(graph, numbers, handler);
    DOMUS_ASSERT(is_shape_valid(graph, shape), "build_shape_or_add_corner: shape is not valid");
    return shape;
}

} // namespace domus::orthogonal::shape