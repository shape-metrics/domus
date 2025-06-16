#include "orthogonal/shape/shape_builder.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "core/tree/tree.hpp"
#include "core/tree/tree_algorithms.hpp"
#include "orthogonal/shape/clauses_functions.hpp"
#include "orthogonal/shape/variables_handler.hpp"
#include "sat/cnf_builder.hpp"
#include "sat/glucose.hpp"

const std::string unit_clauses_logs_file = "unit_clauses_logs.txt";
std::mutex unit_clauses_logs_mutex;

Shape result_to_shape(const Graph& graph, const std::vector<int>& numbers,
                      VariablesHandler& handler) {
  for (int i = 0; i < numbers.size(); i++) {
    int var = numbers[i];
    if (var > 0)
      handler.set_variable_value(var, true);
    else
      handler.set_variable_value(-var, false);
  }
  Shape shape;
  for (const auto& node : graph.get_nodes()) {
    int i = node.get_id();
    for (auto& edge : node.get_edges()) {
      int j = edge.get_to().get_id();
      shape.set_direction(i, j, handler.get_direction_of_edge(i, j));
    }
  }
  return std::move(shape);
}

std::pair<int, int> find_edges_to_split(
    const std::vector<std::string>& proof_lines, std::mt19937& random_engine,
    const VariablesHandler& handler, const Graph& graph) {
  std::vector<int> unit_clauses;
  for (int i = proof_lines.size() - 1; i >= 0; i--) {
    const std::string& line = proof_lines[i];
    // split line based on " "
    std::vector<int> tokens;
    std::string token;
    for (char c : line) {
      if (c == 'd') continue;
      if (c == ' ') {
        if (token == "") continue;
        tokens.push_back(std::stoi(token));
        token = "";
      } else
        token += c;
    }
    if (token != "0") throw std::runtime_error("Invalid proof line");
    if (tokens.size() == 1) unit_clauses.push_back(tokens[0]);
  }
  if (unit_clauses.size() == 0) {
    for (auto line : proof_lines) std::cout << line << "\n";
    throw std::runtime_error("Could not find the edge to remove");
  }
  // pick one of the first two unit clauses
  int random_index = random_engine() % std::min((int)unit_clauses.size(), 2);
  int variable = std::abs(unit_clauses[random_index]);
  std::lock_guard<std::mutex> lock(unit_clauses_logs_mutex);
  std::ofstream log_file(unit_clauses_logs_file, std::ios_base::app);
  if (log_file) {
    log_file << "units " << unit_clauses.size() << "\n";
    log_file.close();
  } else {
    throw std::runtime_error("Error: Could not open log file for writing: " +
                             unit_clauses_logs_file);
  }
  return handler.get_edge_of_variable(variable);
}

std::optional<Shape> build_shape_or_add_corner(Graph& graph,
                                               GraphAttributes& attributes,
                                               std::vector<Cycle>& cycles,
                                               std::mt19937& random_engine);

Shape build_shape(Graph& graph, GraphAttributes& attributes,
                  std::vector<Cycle>& cycles, bool randomize) {
  int seed = (randomize) ? std::random_device{}() : 42;
  std::mt19937 random_engine(seed);
  auto shape =
      build_shape_or_add_corner(graph, attributes, cycles, random_engine);
  while (!shape.has_value())
    shape = build_shape_or_add_corner(graph, attributes, cycles, random_engine);
  return std::move(shape.value());
}

void add_corner_inside_edge(int from_id, int to_id, Graph& graph,
                            GraphAttributes& attributes,
                            std::vector<Cycle>& cycles) {
  int new_node_id = graph.add_node().get_id();
  attributes.set_node_color(new_node_id, Color::RED);
  graph.remove_undirected_edge(from_id, to_id);
  graph.add_undirected_edge(from_id, new_node_id);
  graph.add_undirected_edge(to_id, new_node_id);
  for (Cycle& cycle : cycles) {
    for (int k = 0; k < cycle.size(); k++) {
      if (cycle[k] == from_id && cycle[k + 1] == to_id) {
        cycle.insert(k + 1, new_node_id);
        break;
      }
      if (cycle[k] == to_id && cycle[k + 1] == from_id) {
        cycle.insert(k + 1, new_node_id);
        break;
      }
    }
  }
}

std::optional<Shape> build_shape_or_add_corner(Graph& graph,
                                               GraphAttributes& attributes,
                                               std::vector<Cycle>& cycles,
                                               std::mt19937& random_engine) {
  VariablesHandler handler(graph);
  CnfBuilder cnf_builder;
  cnf_builder.add_comment("constraints one direction per edge");
  add_constraints_one_direction_per_edge(graph, cnf_builder, handler);
  cnf_builder.add_comment("constraints nodes");
  add_nodes_constraints(graph, cnf_builder, handler);
  cnf_builder.add_comment("constraints cycles");
  add_cycles_constraints(graph, cnf_builder, cycles, handler);
  const std::string cnf = get_unique_filename("cnf");
  cnf_builder.convert_to_cnf(cnf);
  auto results = launch_glucose(cnf, false);
  remove(cnf.c_str());
  if (results.result == GlucoseResultType::UNSAT) {
    auto edge =
        find_edges_to_split(results.proof_lines, random_engine, handler, graph);
    add_corner_inside_edge(edge.first, edge.second, graph, attributes, cycles);
    return std::nullopt;
  }
  const std::vector<int>& variables = results.numbers;
  return result_to_shape(graph, variables, handler);
}