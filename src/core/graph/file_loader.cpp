#include "domus/core/graph/file_loader.hpp"

#include <expected>
#include <fstream>
#include <sstream>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/utils.hpp"

using namespace std;
using namespace std::filesystem;

expected<UndirectedGraph, string> load_graph_from_txt_file(path path) {
    UndirectedGraph graph;
    std::ifstream infile(path);
    if (!infile) {
        string error = "Could not load graph from file.\nCannot open: " + path.string();
        return std::unexpected(error);
    }
    string line;
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
                if (iss >> node_id)
                    graph.add_node(node_id);
            } else if (section == EDGES) {
                int from, to;
                if (iss >> from >> to)
                    graph.add_edge(from, to);
            }
        }
    }
    return graph;
}

std::expected<void, std::string> save_graph_to_file(const UndirectedGraph& graph, path path) {
    std::ofstream outfile(path);
    if (!outfile) {
        string error = "Could not save graph to file.\n";
        error += "could not write to file: " + path.string();
        return std::unexpected(error);
    }
    outfile << "nodes:\n";
    for (const int node_id : graph.get_nodes_ids())
        outfile << node_id << '\n';
    outfile << "edges:\n";
    for (int node_id : graph.get_nodes_ids())
        for (int neighbor_id : graph.get_neighbors_of_node(node_id))
            if (neighbor_id > node_id)
                outfile << node_id << ' ' << neighbor_id << '\n';
    return {};
}

void write_data_tag(std::ostream& os, string key_id, string value) {
    os << "    <data key=\"" << key_id << "\">" << value << "</data>\n";
}

void save_to_graphml(
    std::ostream& os, const UndirectedGraph& graph, const GraphAttributes& attributes
) {
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    os << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"\n";
    os << "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n";
    os << "         "
          "xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns\n";
    os << "         "
          "http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">\n\n";
    if (attributes.has_attribute(Attribute::NODES_COLOR))
        os << "  <key id=\"d0\" for=\"node\" attr.name=\"color\" "
              "attr.type=\"string\"/>\n";

    if (attributes.has_attribute(Attribute::NODES_POSITION)) {
        os << "  <key id=\"d1\" for=\"node\" attr.name=\"pos_x\" "
              "attr.type=\"int\"/>\n";
        os << "  <key id=\"d2\" for=\"node\" attr.name=\"pos_y\" "
              "attr.type=\"int\"/>\n";
    }
    os << "\n";
    os << "  <graph id=\"G\" edgedefault=\"undirected\">\n";
    for (int node_id : graph.get_nodes_ids()) {
        os << "    <node id=\"n" << node_id << "\">\n";
        if (attributes.has_attribute(Attribute::NODES_COLOR)) {
            const Color color = attributes.get_node_color(node_id);
            write_data_tag(os, "d0", color_to_string(color));
        }
        if (attributes.has_attribute(Attribute::NODES_POSITION)) {
            write_data_tag(os, "d1", std::to_string(attributes.get_position_x(node_id)));
            write_data_tag(os, "d2", std::to_string(attributes.get_position_y(node_id)));
        }
        os << "    </node>\n";
    }
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (neighbor_id > node_id)
                continue;
            os << "    <source=\"n" << node_id << "\" target=\"n" << neighbor_id << "\">\n";
            os << "    </edge>\n";
        }
    }
    os << "\n";
    os << "  </graph>\n";
    os << "</graphml>\n";
}

std::expected<void, std::string> save_graph_to_graphml_file(
    const UndirectedGraph& graph, const GraphAttributes& attributes, path path
) {
    std::ofstream outfile(path);
    if (!outfile) {
        string error = "Could not save graph to file.\n";
        error += "could not write to file: " + path.string();
        return std::unexpected(error);
    }
    save_to_graphml(outfile, graph, attributes);
    return {};
}