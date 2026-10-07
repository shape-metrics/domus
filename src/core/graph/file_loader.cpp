#include "domus/core/graph/file_loader.hpp"

#include <algorithm>
#include <expected>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

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

std::expected<void, std::string>
save_embedding_to_file(const Embedding& embedding, std::filesystem::path path) {
    std::ofstream outfile(path);
    if (!outfile) {
        return std::unexpected(
            std::format("save_embedding_to_file: could not write to file: {}", path.string())
        );
    }
    outfile << "nodes:\n";
    for (const size_t node_id : embedding.get_nodes_ids()) {
        outfile << node_id << '\n';
    }

    outfile << "embedding:\n";
    for (const size_t node_id : embedding.get_nodes_ids()) {
        outfile << node_id << ":";
        for (const size_t neighbor_id : embedding.get_neighbors(node_id)) {
            outfile << ' ' << neighbor_id;
        }
        outfile << '\n';
    }
    return {};
}

struct ParsedEmbeddingData {
    std::vector<size_t> nodes;
    std::map<size_t, std::vector<size_t>> adj_map;
    size_t num_nodes = 0;
};

static std::expected<ParsedEmbeddingData, std::string>
parse_embedding_file(const std::filesystem::path& path) {
    std::ifstream infile(path);
    if (!infile) {
        return std::unexpected(
            std::format("load_embedding_from_file: cannot open: {}", path.string())
        );
    }

    ParsedEmbeddingData data;
    enum Section { NONE, NODES, EMBEDDING } section = NONE;
    std::string line;

    while (std::getline(infile, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        if (line == "nodes:" || line == "NODES:") {
            section = NODES;
            continue;
        } else if (line == "embedding:" || line == "Embedding:" || line == "EMBEDDING:") {
            section = EMBEDDING;
            continue;
        }

        const auto colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string left_str = line.substr(0, colon_pos);
            std::istringstream left_iss(left_str);
            size_t node_id{};
            if (left_iss >> node_id) {
                std::string right_str = line.substr(colon_pos + 1);
                std::istringstream right_iss(right_str);
                std::string token;
                std::vector<size_t> neighbors;
                while (right_iss >> token) {
                    if (token == "[" || token == "]")
                        continue;
                    try {
                        size_t neighbor_id = std::stoull(token);
                        neighbors.push_back(neighbor_id);
                    } catch (...) {
                        return std::unexpected(
                            std::format(
                                "load_embedding_from_file: invalid neighbor token '{}' in {}",
                                token,
                                path.string()
                            )
                        );
                    }
                }
                data.adj_map[node_id] = std::move(neighbors);
                continue;
            }
        }

        if (section == NODES) {
            std::istringstream iss(line);
            size_t node_id{};
            if (iss >> node_id) {
                data.nodes.push_back(node_id);
            }
        }
    }

    for (const size_t n : data.nodes)
        data.num_nodes = std::max(data.num_nodes, n + 1);
    for (const auto& [u, nbrs] : data.adj_map) {
        data.num_nodes = std::max(data.num_nodes, u + 1);
        for (const size_t v : nbrs)
            data.num_nodes = std::max(data.num_nodes, v + 1);
    }

    return data;
}

std::expected<Embedding, std::string> load_embedding_from_file(std::filesystem::path path) {
    auto parsed = parse_embedding_file(path);
    if (!parsed)
        return std::unexpected(parsed.error());

    const auto& [nodes, adj_map, num_nodes] = *parsed;

    Embedding embedding;
    while (embedding.get_number_of_nodes() < num_nodes)
        embedding.add_node();

    std::map<std::pair<size_t, size_t>, size_t> edge_ids;
    size_t next_edge_id = 0;

    for (const auto& [u, nbrs] : adj_map) {
        for (const size_t v : nbrs) {
            if (u == v) {
                return std::unexpected(
                    std::format(
                        "load_embedding_from_file: self-loop on node {} in {}",
                        u,
                        path.string()
                    )
                );
            }
            auto edge_key = std::make_pair(std::min(u, v), std::max(u, v));
            if (!edge_ids.contains(edge_key)) {
                edge_ids[edge_key] = next_edge_id++;
            }
        }
    }

    for (size_t u = 0; u < num_nodes; ++u) {
        auto it = adj_map.find(u);
        if (it != adj_map.end()) {
            for (const size_t v : it->second) {
                auto edge_key = std::make_pair(std::min(u, v), std::max(u, v));
                size_t edge_id = edge_ids.at(edge_key);
                embedding.add_edge(u, v, edge_id);
            }
        }
    }

    if (!embedding.is_consistent()) {
        return std::unexpected(
            std::format(
                "load_embedding_from_file: embedding is not consistent (asymmetric edges) in {}",
                path.string()
            )
        );
    }

    return embedding;
}

std::expected<Embedding, std::string>
load_embedding_from_file(const Graph& graph, std::filesystem::path path) {
    auto parsed = parse_embedding_file(path);
    if (!parsed)
        return std::unexpected(parsed.error());

    const auto& [nodes, adj_map, num_nodes] = *parsed;

    if (num_nodes != graph.get_number_of_nodes()) {
        return std::unexpected(
            std::format(
                "load_embedding_from_file: node count mismatch (file has {}, graph has {}) in {}",
                num_nodes,
                graph.get_number_of_nodes(),
                path.string()
            )
        );
    }

    Embedding embedding(graph);

    for (size_t u = 0; u < num_nodes; ++u) {
        auto it = adj_map.find(u);
        if (it != adj_map.end()) {
            for (const size_t v : it->second) {
                if (u == v) {
                    return std::unexpected(
                        std::format(
                            "load_embedding_from_file: self-loop on node {} in {}",
                            u,
                            path.string()
                        )
                    );
                }
                std::optional<size_t> found_edge_id;
                for (const auto edge : graph.get_edges(u)) {
                    if (edge.neighbor_id == v) {
                        found_edge_id = edge.id;
                        break;
                    }
                }
                if (!found_edge_id) {
                    return std::unexpected(
                        std::format(
                            "load_embedding_from_file: edge ({}, {}) does not exist in graph",
                            u,
                            v
                        )
                    );
                }
                embedding.add_edge(u, v, *found_edge_id);
            }
        }
    }

    if (!embedding.is_consistent()) {
        return std::unexpected(
            std::format(
                "load_embedding_from_file: embedding is not consistent in {}",
                path.string()
            )
        );
    }

    return embedding;
}

} // namespace domus::graph::loader