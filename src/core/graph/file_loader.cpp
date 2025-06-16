#include "core/graph/file_loader.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::unique_ptr<Graph> load_graph_from_txt_file(const std::string& filename) {
  auto graph = std::make_unique<Graph>();
  load_graph_from_txt_file(filename, *graph);
  return std::move(graph);
}

void load_graph_from_txt_file(const std::string& filename, Graph& graph) {
  if (graph.size() > 0)
    throw std::runtime_error("Graph is not empty. Please use a new graph.");
  std::ifstream infile(filename);
  if (!infile) throw std::runtime_error("Could not open file: " + filename);
  std::string line;
  enum Section { NONE, NODES, EDGES } section = NONE;
  while (std::getline(infile, line)) {
    if (line == "nodes:") {
      section = NODES;
    } else if (line == "edges:") {
      section = EDGES;
    } else if (!line.empty()) {
      std::istringstream iss(line);
      if (section == NODES) {
        int node_id;
        if (iss >> node_id) graph.add_node(node_id);
      } else if (section == EDGES) {
        int from, to;
        if (iss >> from >> to) graph.add_edge(from, to);
      }
    }
  }
  infile.close();
}

std::unique_ptr<Graph> load_undirected_graph_from_gml_file(
    const std::string& filename) {
  auto graph = std::make_unique<Graph>();
  std::ifstream file(filename);
  if (!file.is_open())
    throw std::runtime_error("Failed to open file: " + filename);
  std::string line;
  bool in_node_block = false, in_edge_block = false;
  int node_id = -1, source = -1, target = -1;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;
    if (token == "node") {
      in_node_block = true;
      node_id = -1;
    } else if (token == "edge") {
      in_edge_block = true;
      source = target = -1;
    } else if (token == "id" && in_node_block) {
      iss >> node_id;
      graph->add_node(node_id);  // Add node to graph
    } else if (token == "source" && in_edge_block)
      iss >> source;
    else if (token == "target" && in_edge_block)
      iss >> target;
    else if (token == "]") {
      if (in_node_block)
        in_node_block = false;
      else if (in_edge_block) {
        if (source != -1 && target != -1)
          graph->add_undirected_edge(source, target);
        in_edge_block = false;
      }
    }
  }
  file.close();
  return graph;
}

void save_graph_to_file(const Graph& graph, const std::string& filename) {
  std::ofstream outfile(filename);
  if (!outfile)
    throw std::runtime_error("Could not write to file: " + filename);
  outfile << "nodes:\n";
  for (auto node : graph.get_nodes()) outfile << node.get_id() << '\n';
  outfile << "edges:\n";
  for (const auto& edge : graph.get_edges())
    outfile << edge.get_from().get_id() << ' ' << edge.get_to().get_id()
            << '\n';
  outfile.close();
}
