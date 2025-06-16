#ifndef MY_GRAPH_FILE_LOADER_H
#define MY_GRAPH_FILE_LOADER_H

#include <memory>
#include <string>
#include <vector>

#include "core/graph/graph.hpp"

std::unique_ptr<Graph> load_graph_from_txt_file(const std::string& filename);

void load_graph_from_txt_file(const std::string& filename, Graph& graph);

std::unique_ptr<Graph> load_undirected_graph_from_gml_file(
    const std::string& filename);

void save_graph_to_file(const Graph& graph, const std::string& filename);

#endif