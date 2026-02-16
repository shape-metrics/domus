#ifndef MY_GRAPH_FILE_LOADER_H
#define MY_GRAPH_FILE_LOADER_H

#include <string>

#include "domus/core/graph/graph.hpp"

class GraphAttributes;

UndirectedGraph load_graph_from_txt_file(std::string filename);

void load_graph_from_txt_file(std::string filename, UndirectedGraph& graph);

void save_graph_to_file(const UndirectedGraph& graph, std::string filename);

void save_graph_to_graphml_file(
    const UndirectedGraph& graph, const GraphAttributes& attributes, std::string filename
);

#endif