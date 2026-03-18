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
#include "domus/sat/cnf.hpp"
#include "domus/sat/sat.hpp"

#include "../../core/domus_assert.hpp"
#include "clauses_functions.hpp"
#include "variables_handler.hpp"

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
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            shape.set_direction(
                node_id,
                neighbor_id,
                handler.get_direction_of_edge(node_id, neighbor_id)
            );
        });
    });
    return shape;
}

Edge find_edges_to_split(
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
    return handler.get_edge_of_variable(variable);
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
    std::optional<Shape> shape =
        build_shape_or_add_corner(graph, attributes, cycles, random_engine);
    while (!shape.has_value())
        shape = build_shape_or_add_corner(graph, attributes, cycles, random_engine);
    return std::move(shape.value());
}

void add_corner_inside_edge(
    size_t from_id,
    size_t to_id,
    Graph& graph,
    GraphAttributes& attributes,
    std::vector<Cycle>& cycles
) {
    DOMUS_ASSERT(graph.are_neighbors(from_id, to_id), "add_corner_inside_edge: not neighbors");
    size_t new_node_id = graph.add_node();
    attributes.set_node_color(new_node_id, Color::RED);
    graph.remove_edge(from_id, to_id);
    graph.add_edge(from_id, new_node_id);
    graph.add_edge(to_id, new_node_id);
    for (Cycle& cycle : cycles) {
        if (!cycle.has_node(from_id) || !cycle.has_node(to_id))
            continue;
        size_t from_pos = cycle.node_position(from_id);
        size_t to_pos = cycle.node_position(to_id);
        if (cycle.at(from_pos + 1) == to_id)
            cycle.insert(to_pos, new_node_id);
        else if (cycle.at(to_pos + 1) == from_id)
            cycle.insert(from_pos, new_node_id);
    }
}

std::optional<Shape> build_shape_or_add_corner(
    Graph& graph,
    GraphAttributes& attributes,
    std::vector<Cycle>& cycles,
    std::mt19937& random_engine
) {
    VariablesHandler handler(graph);
    Cnf cnf{};
    // cnf.add_comment("constraints one direction per edge");
    add_constraints_one_direction_per_edge(graph, cnf, handler);
    // cnf.add_comment("constraints nodes");
    add_nodes_constraints(graph, cnf, handler);
    // cnf.add_comment("constraints cycles");
    add_cycles_constraints(cnf, cycles, handler);
    const auto [result, numbers, proof_lines] = launch_glucose(cnf);
    if (result == SatSolverResultType::UNSAT) {
        const auto [from_id, to_id] =
            find_edges_to_split(proof_lines, random_engine, handler, cnf.get_number_of_variables());
        add_corner_inside_edge(from_id, to_id, graph, attributes, cycles);
        return std::nullopt;
    }
    return result_to_shape(graph, numbers, handler);
}