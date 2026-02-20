#include "domus/orthogonal/shape/shape_builder.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/utils.hpp"
#include "domus/orthogonal/shape/clauses_functions.hpp"
#include "domus/orthogonal/shape/variables_handler.hpp"
#include "domus/sat/cnf.hpp"
#include "domus/sat/sat.hpp"

using namespace std;

const string unit_clauses_logs_file = "unit_clauses_logs.txt";
std::mutex unit_clauses_logs_mutex;

Shape result_to_shape(
    const UndirectedGraph& graph, const vector<int>& numbers, VariablesHandler& handler
) {
    for (const int var : numbers) {
        if (var > 0)
            handler.set_variable_value(var, true);
        else
            handler.set_variable_value(-var, false);
    }
    Shape shape;
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            shape.set_direction(
                node_id,
                neighbor_id,
                handler.get_direction_of_edge(node_id, neighbor_id)
            );
        }
    }
    return shape;
}

pair<int, int> find_edges_to_split(
    const vector<string>& proof_lines,
    std::mt19937& random_engine,
    const VariablesHandler& handler,
    int number_of_variables
) {
    vector<int> unit_clauses;
    for (size_t i = proof_lines.size(); i > 0; i--) {
        const string& line = proof_lines[i - 1];
        // split line based on " "
        vector<int> tokens;
        string token;
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
        if (token != "0")
            throw runtime_error("Invalid proof line");
        if (tokens.size() == 1) {
            int unit_clause = tokens[0];
            if (std::abs(unit_clause) <= number_of_variables)
                unit_clauses.push_back(unit_clause);
        }
    }
    if (unit_clauses.empty())
        throw runtime_error("Could not find the edge to remove");
    // pick one of the first two unit clauses
    size_t random_index = random_engine() % min(unit_clauses.size(), static_cast<size_t>(2));
    const int variable = std::abs(unit_clauses[random_index]);
    std::lock_guard lock(unit_clauses_logs_mutex);
    std::ofstream log_file(unit_clauses_logs_file, std::ios_base::app);
    if (log_file) {
        log_file << "units " << unit_clauses.size() << "\n";
        log_file.close();
    } else {
        throw runtime_error(
            "Error: Could not open log file for writing: " + unit_clauses_logs_file
        );
    }
    return handler.get_edge_of_variable(variable);
}

optional<Shape> build_shape_or_add_corner(
    UndirectedGraph& graph,
    GraphAttributes& attributes,
    vector<Cycle>& cycles,
    std::mt19937& random_engine
);

Shape build_shape(
    UndirectedGraph& graph, GraphAttributes& attributes, vector<Cycle>& cycles, const bool randomize
) {
    const size_t seed = randomize ? std::random_device{}() : 42;
    std::mt19937 random_engine(seed);
    optional<Shape> shape = build_shape_or_add_corner(graph, attributes, cycles, random_engine);
    while (!shape.has_value())
        shape = build_shape_or_add_corner(graph, attributes, cycles, random_engine);
    return std::move(shape.value());
}

void add_corner_inside_edge(
    const int from_id,
    const int to_id,
    UndirectedGraph& graph,
    GraphAttributes& attributes,
    vector<Cycle>& cycles
) {
    assert(graph.has_edge(from_id, to_id));
    const int new_node_id = graph.add_node();
    attributes.set_node_color(new_node_id, Color::RED);
    graph.remove_edge(from_id, to_id);
    graph.add_edge(from_id, new_node_id);
    graph.add_edge(to_id, new_node_id);
    for (Cycle& cycle : cycles) {
        if (!cycle.has_node(from_id) || !cycle.has_node(to_id))
            continue;
        const size_t from_pos = cycle.node_position(from_id);
        const size_t to_pos = cycle.node_position(to_id);
        if (cycle.next_of_node(from_id) == to_id)
            cycle.insert(to_pos, new_node_id);
        else if (cycle.next_of_node(to_id) == from_id)
            cycle.insert(from_pos, new_node_id);
    }
}

optional<Shape> build_shape_or_add_corner(
    UndirectedGraph& graph,
    GraphAttributes& attributes,
    vector<Cycle>& cycles,
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