#include "domus/core/graph/file_loader.hpp"

#include <expected>
#include <fstream>
#include <sstream>

#include "domus/core/color.hpp"
#include "domus/core/graph/attributes.hpp"

namespace domus::graph::loader {
using color::ColorRGB;

std::expected<Graph, std::string> load_graph_from_txt_file(std::filesystem::path path) {
    Graph graph;
    std::vector<size_t> nodes;
    std::ifstream infile(path);
    if (!infile) {
        return std::unexpected(
            std::format("load_graph_from_txt_file: cannot open: {}", path.string())
        );
    }
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
                size_t node_id;
                if (iss >> node_id) {
                    nodes.push_back(node_id);
                    graph.add_node();
                }
            } else if (section == EDGES) {
                size_t from, to;
                if (iss >> from >> to)
                    graph.add_edge(from, to);
            }
        }
    }
    for (size_t node_id : nodes)
        if (!graph.has_node(node_id))
            return std::unexpected("load_graph_from_txt_file: invalid graph");
    return graph;
}

std::expected<void, std::string>
save_graph_to_file(const Graph& graph, std::filesystem::path path) {
    std::ofstream outfile(path);
    if (!outfile) {
        return std::unexpected(
            std::format("save_graph_to_file: could not write to file: {}", path.string())
        );
    }
    outfile << "nodes:\n";
    for (const size_t node_id : graph.get_nodes_ids())
        outfile << node_id << '\n';

    outfile << "edges:\n";
    for (const EdgeId edge : graph.get_all_edges())
        outfile << edge.edge.from_id << ' ' << edge.edge.to_id << '\n';
    return {};
}

void write_data_tag(std::ostream& os, std::string key_id, std::string value) {
    os << "    <data key=\"" << key_id << "\">" << value << "</data>\n";
}

void save_to_graphml(std::ostream& os, const Graph& graph, const Attributes& attributes) {
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
    for (const size_t node_id : graph.get_nodes_ids()) {
        os << "    <node id=\"n" << node_id << "\">\n";
        if (attributes.has_attribute(Attribute::NODES_COLOR)) {
            const ColorRGB color = attributes.get_node_color(node_id);
            write_data_tag(os, "d0", color_to_string(color));
        }
        if (attributes.has_attribute(Attribute::NODES_POSITION)) {
            write_data_tag(os, "d1", std::to_string(attributes.get_position_x(node_id)));
            write_data_tag(os, "d2", std::to_string(attributes.get_position_y(node_id)));
        }
        os << "    </node>\n";
    }
    for (const EdgeId edge : graph.get_all_edges()) {
        os << "    <source=\"n" << edge.edge.from_id << "\" target=\"n" << edge.edge.to_id
           << "\">\n";
        os << "    </edge>\n";
    }
    os << "\n";
    os << "  </graph>\n";
    os << "</graphml>\n";
}

std::expected<void, std::string> save_graph_to_graphml_file(
    const Graph& graph, const Attributes& attributes, std::filesystem::path path
) {
    std::ofstream outfile(path);
    if (!outfile) {
        return std::unexpected(
            std::format("save_graph_to_graphml_file: could not write to file: {}", path.string())
        );
    }
    save_to_graphml(outfile, graph, attributes);
    return {};
}

// Nodes in the file are 1-based; they get internally converted to 0-based.
//
// The .asc format for each graph block is:
//
//   Graph N:
//
//   1 : v1 v2 ...       <- adjacency section
//   ...
//   Taillenweite: K     <- separator: adjacency ends here
//
//   a : p1 p2 ...       <- automorphism permutations (ignored)
//   Ordnung: M
//
// We use "Taillenweite:" as the boundary: only lines between "Graph N:" and
// "Taillenweite:" are parsed as adjacency rows. This avoids any assumption
// about the number of nodes.
std::expected<std::vector<Graph>, std::string>
load_graphs_from_asc_file(const std::filesystem::path& path) {
    std::ifstream infile(path);
    if (!infile)
        return std::unexpected(
            std::format("load_graphs_from_asc_file: cannot open: {}", path.string())
        );

    std::vector<Graph> graphs;
    Graph* current = nullptr;
    bool in_adjacency_section = false;
    std::string line;

    while (std::getline(infile, line)) {
        // Strip trailing CR (Windows line endings)
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        // New graph block
        if (line.rfind("Graph ", 0) == 0) {
            graphs.emplace_back();
            current = &graphs.back();
            in_adjacency_section = true;
            continue;
        }

        if (current == nullptr)
            continue;

        // "Taillenweite:" marks the end of the adjacency section
        if (line.rfind("Taillenweite:", 0) == 0) {
            in_adjacency_section = false;
            continue;
        }

        if (!in_adjacency_section)
            continue;

        // Adjacency line must start with a digit
        if (!std::isdigit(static_cast<unsigned char>(line[0])))
            continue;

        const auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
            continue;

        std::istringstream left_ss(line.substr(0, colon_pos));
        size_t node_1based{};
        if (!(left_ss >> node_1based))
            continue;

        std::istringstream right_ss(line.substr(colon_pos + 1));
        std::vector<size_t> neighbors;
        size_t v;
        while (right_ss >> v)
            neighbors.push_back(v);

        // Ensure the source node exists
        while (current->get_number_of_nodes() < node_1based)
            current->add_node();

        const size_t from = node_1based - 1; // convert to 0-based
        for (const size_t neighbor_1based : neighbors) {
            const size_t to = neighbor_1based - 1;
            // Ensure the destination node exists
            while (current->get_number_of_nodes() <= to)
                current->add_node();
            // Undirected: add each edge only once
            if (from < to)
                current->add_edge(from, to);
        }
    }

    return graphs;
}

} // namespace domus::graph::loader