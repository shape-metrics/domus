#include "domus/core/graph/file_loader.hpp"

#include <expected>
#include <fstream>
#include <sstream>

#include "domus/core/graph/attributes.hpp"

namespace domus::graph::loader {

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
    graph.for_each_node([&outfile](size_t node_id) { outfile << node_id << '\n'; });
    outfile << "edges:\n";
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (neighbor_id > node_id)
                outfile << node_id << ' ' << neighbor_id << '\n';
        });
    });
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
    graph.for_each_node([&](size_t node_id) {
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
    });
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [node_id, &os](size_t neighbor_id) {
            if (neighbor_id > node_id)
                return;
            os << "    <source=\"n" << node_id << "\" target=\"n" << neighbor_id << "\">\n";
            os << "    </edge>\n";
        });
    });
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

} // namespace domus::graph::loader