#include "domus/ogdf_utils.hpp"

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/List.h>
#include <ogdf/planarity/BoyerMyrvold.h>

#include "domus/core/debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"

namespace domus::ogdf_utils {
using namespace domus::graph;

std::vector<ogdf::node> to_ogdf_graph(const Graph& graph, ogdf::Graph& G) {
    std::vector<ogdf::node> nodes;
    nodes.reserve(graph.get_number_of_nodes());

    for (size_t i = 0; i < graph.get_number_of_nodes(); ++i) {
        nodes.push_back(G.newNode());
    }

    for (const auto& edge : graph.get_all_edges()) {
        G.newEdge(nodes[edge.edge.from_id], nodes[edge.edge.to_id], static_cast<int>(edge.id));
    }

    return nodes;
}

std::vector<size_t> find_kuratowski_subdivision(const Graph& graph) {
    ogdf::Graph G;
    to_ogdf_graph(graph, G);

    ogdf::BoyerMyrvold bm;
    ogdf::SList<ogdf::KuratowskiWrapper> kuratowski;

    std::vector<size_t> edge_ids_kuratowski;

    if (bm.planarEmbed(G, kuratowski, 1)) {
        DOMUS_ASSERT(false, "find_kuratowski_subdivision: graph is planar");
    } else {
        for (auto k : kuratowski) {
            for (auto e : k.edgeList) {
                edge_ids_kuratowski.push_back(static_cast<size_t>(e->index()));
            }
        }
    }

    return edge_ids_kuratowski;
}

std::optional<Embedding> compute_planar_embedding(const Graph& graph) {
    if (graph.get_number_of_nodes() < 4) {
        Embedding embedding(graph);
        for (const size_t node_id : graph.get_nodes_ids()) {
            for (const auto edge : graph.get_edges(node_id)) {
                embedding.add_edge(node_id, edge.neighbor_id, edge.id);
            }
        }
        DOMUS_ASSERT(
            is_embedding_planar(embedding),
            "compute_planar_embedding: output embedding is not planar"
        );
        return embedding;
    }

    ogdf::Graph G;
    const std::vector<ogdf::node> ogdf_nodes = to_ogdf_graph(graph, G);

    ogdf::BoyerMyrvold bm;
    if (!bm.planarEmbed(G)) {
        return std::nullopt;
    }

    Embedding embedding(graph);
    for (size_t node_id = 0; node_id < graph.get_number_of_nodes(); ++node_id) {
        const ogdf::node v = ogdf_nodes[node_id];
        for (const ogdf::adjEntry adj : v->adjEntries) {
            const size_t neighbor_id = static_cast<size_t>(adj->twinNode()->index());
            const size_t edge_id = static_cast<size_t>(adj->theEdge()->index());
            embedding.add_edge(node_id, neighbor_id, edge_id);
        }
    }

    DOMUS_ASSERT(
        is_embedding_planar(embedding),
        "compute_planar_embedding: output embedding is not planar"
    );

    return embedding;
}

bool is_graph_planar(const Graph& graph) {
    if (graph.get_number_of_nodes() < 4) {
        return true;
    }

    ogdf::Graph G;
    to_ogdf_graph(graph, G);

    ogdf::BoyerMyrvold bm;
    return bm.isPlanarDestructive(G);
}

} // namespace domus::ogdf_utils