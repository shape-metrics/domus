#include "domus/core/graph/embedding.hpp"

#include <algorithm>
#include <cstddef>
#include <print>

#include "domus/core/graph/concept.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::graph {
using namespace graph::utilities;

Embedding::Embedding() {}

Embedding::Embedding(const Graph& graph) {
    for (size_t i = 0; i < graph.get_number_of_nodes(); i++)
        m_adjacency_list.push_back({});
}

size_t Embedding::add_node() {
    m_adjacency_list.push_back({});
    return m_adjacency_list.size() - 1;
}

bool Embedding::has_node(size_t node_id) const { return node_id < get_number_of_nodes(); }

bool Embedding::are_neighbors(size_t node_1_id, size_t node_2_id) const {
    DOMUS_ASSERT(node_1_id != node_2_id, "Embedding::are_neighbors: nodes are equal");
    for (EdgeIter edge : m_adjacency_list.at(node_1_id))
        if (edge.neighbor_id == node_2_id)
            return true;
    for (EdgeIter edge : m_adjacency_list.at(node_2_id))
        if (edge.neighbor_id == node_1_id)
            return true;
    return false;
}

EdgeIter
Embedding::next_in_adjacency_list(size_t node_id, size_t neighbor_id, size_t edge_id) const {
    DOMUS_ASSERT(
        m_next_in_adjacency_list.has_label(node_id, neighbor_id, edge_id),
        "Embedding::next_in_adjacency_list: edge does not exist"
    );
    return m_next_in_adjacency_list.get_label(node_id, neighbor_id, edge_id);
}

EdgeIter
Embedding::prev_in_adjacency_list(size_t node_id, size_t neighbor_id, size_t edge_id) const {
    DOMUS_ASSERT(
        m_prev_in_adjacency_list.has_label(node_id, neighbor_id, edge_id),
        "Embedding::prev_in_adjacency_list: edge does not exist"
    );
    return m_prev_in_adjacency_list.get_label(node_id, neighbor_id, edge_id);
}

size_t Embedding::get_degree_of_node(size_t node_id) const {
    return m_adjacency_list.at(node_id).size();
}

void Embedding::add_edge(size_t from_id, size_t to_id, size_t edge_id) {
    EdgeIter new_edge{edge_id, to_id};
    auto& adj = m_adjacency_list.at(from_id);

    if (adj.empty()) {
        m_next_in_adjacency_list.add_label(from_id, to_id, edge_id, new_edge);
        m_prev_in_adjacency_list.add_label(from_id, to_id, edge_id, new_edge);
    } else {
        auto [first_edge_id, first_to_id] = adj.front();
        auto [last_edge_id, last_to_id] = adj.back();

        // updating old prevs and nexts
        m_prev_in_adjacency_list.update_label(from_id, first_to_id, first_edge_id, new_edge);
        m_next_in_adjacency_list.update_label(from_id, last_to_id, last_edge_id, new_edge);

        // adding prev and next of new edge
        m_next_in_adjacency_list.add_label(from_id, to_id, edge_id, {first_edge_id, first_to_id});
        m_prev_in_adjacency_list.add_label(from_id, to_id, edge_id, {last_edge_id, last_to_id});
    }

    adj.push_back(new_edge);
    m_number_of_edges++;
}

void Embedding::add_edge_after(size_t from_id, size_t to_id, size_t edge_id, size_t prev_edge_id) {
    DOMUS_ASSERT(
        get_degree_of_node(from_id) > 1,
        "Embedding::add_edge_after: from_id has degree <= 1"
    );
    DOMUS_ASSERT(from_id != to_id, "Embedding::add_edge_after: from_id and to_id are equal");
    DOMUS_ASSERT(
        has_node(from_id) && has_node(to_id),
        "Embedding::add_edge_after: node does not exist"
    );

    EdgeIter new_edge{edge_id, to_id};
    auto& adj = m_adjacency_list.at(from_id);

    // find the position of prev_edge_id in the adjacency list
    auto prev_it = std::ranges::find_if(adj, [prev_edge_id](const EdgeIter& e) {
        return e.id == prev_edge_id;
    });
    DOMUS_ASSERT(prev_it != adj.end(), "Embedding::add_edge_after: prev_edge_id not found");

    size_t prev_to_id = prev_it->neighbor_id;

    // get the next edge after prev_edge
    DOMUS_ASSERT(
        m_next_in_adjacency_list.has_label(from_id, prev_to_id, prev_edge_id),
        "Embedding::add_edge_after: prev_edge_id does not exist"
    );
    EdgeIter next_edge = m_next_in_adjacency_list.get_label(from_id, prev_to_id, prev_edge_id);

    size_t next_edge_id = next_edge.id;
    size_t next_to_id = next_edge.neighbor_id;

    // update prev's next to point to new edge
    m_next_in_adjacency_list.update_label(from_id, prev_to_id, prev_edge_id, new_edge);

    // update next's prev to point to new edge
    m_prev_in_adjacency_list.update_label(from_id, next_to_id, next_edge_id, new_edge);

    // set new edge's prev and next
    m_prev_in_adjacency_list.add_label(from_id, to_id, edge_id, {prev_edge_id, prev_to_id});
    m_next_in_adjacency_list.add_label(from_id, to_id, edge_id, next_edge);

    // insert new edge after prev_it in the adjacency list
    adj.insert(prev_it + 1, new_edge);
    m_number_of_edges++;
}

void Embedding::add_edge_before(size_t from_id, size_t to_id, size_t edge_id, size_t next_edge_id) {
    DOMUS_ASSERT(
        get_degree_of_node(from_id) > 1,
        "Embedding::add_edge_before: from_id has degree <= 1"
    );
    DOMUS_ASSERT(from_id != to_id, "Embedding::add_edge_before: from_id and to_id are equal");
    DOMUS_ASSERT(
        has_node(from_id) && has_node(to_id),
        "Embedding::add_edge_before: node does not exist"
    );

    auto& adj = m_adjacency_list.at(from_id);

    // find the position of next_edge_id in the adjacency list
    auto next_it = std::ranges::find_if(adj, [next_edge_id](const EdgeIter& e) {
        return e.id == next_edge_id;
    });
    DOMUS_ASSERT(next_it != adj.end(), "Embedding::add_edge_before: next_edge_id not found");

    size_t next_to_id = next_it->neighbor_id;

    // get the prev edge before next_edge
    DOMUS_ASSERT(
        m_prev_in_adjacency_list.has_label(from_id, next_to_id, next_edge_id),
        "Embedding::add_edge_before: next_edge_id does not exist"
    );
    EdgeIter prev_edge = m_prev_in_adjacency_list.get_label(from_id, next_to_id, next_edge_id);
    size_t prev_edge_id = prev_edge.id;
    size_t prev_to_id = prev_edge.neighbor_id;

    EdgeIter new_edge{edge_id, to_id};

    // update prev's next to point to new edge
    m_next_in_adjacency_list.update_label(from_id, prev_to_id, prev_edge_id, new_edge);

    // update next's prev to point to new edge
    m_prev_in_adjacency_list.update_label(from_id, next_to_id, next_edge_id, new_edge);

    // set new edge's prev and next
    m_prev_in_adjacency_list.add_label(from_id, to_id, edge_id, {prev_edge_id, prev_to_id});
    m_next_in_adjacency_list.add_label(from_id, to_id, edge_id, {next_edge_id, next_to_id});

    // insert new edge before next_it in the adjacency list
    adj.insert(next_it, new_edge);
    m_number_of_edges++;
}

void Embedding::reverse_circular_order(size_t node_id) {
    DOMUS_ASSERT(has_node(node_id), "Embedding::reverse_circular_order: node does not exist");

    std::vector<domus::graph::EdgeIter>& adj = m_adjacency_list.at(node_id);
    if (adj.size() <= 1)
        return;

    for (const EdgeIter& edge : adj) {
        size_t neighbor_id = edge.neighbor_id;
        size_t edge_id = edge.id;

        EdgeIter next_edge = m_next_in_adjacency_list.get_label(node_id, neighbor_id, edge_id);
        EdgeIter prev_edge = m_prev_in_adjacency_list.get_label(node_id, neighbor_id, edge_id);

        m_next_in_adjacency_list.update_label(node_id, neighbor_id, edge_id, prev_edge);
        m_prev_in_adjacency_list.update_label(node_id, neighbor_id, edge_id, next_edge);
    }

    std::ranges::reverse(adj);
}

void Embedding::remove_edge(size_t from_id, size_t to_id, size_t edge_id) {
    DOMUS_ASSERT(
        has_node(from_id) && has_node(to_id),
        "Embedding::remove_edge: node does not exist"
    );

    // find and remove the edge from the adjacency list
    auto& adj = m_adjacency_list.at(from_id);
    auto it = std::ranges::find_if(adj, [edge_id](const EdgeIter& e) { return e.id == edge_id; });
    DOMUS_ASSERT(it != adj.end(), "Embedding::remove_edge: edge does not exist");

    // update the linked list pointers
    DOMUS_ASSERT(
        m_prev_in_adjacency_list.has_label(from_id, to_id, edge_id) &&
            m_next_in_adjacency_list.has_label(from_id, to_id, edge_id),
        "Embedding::remove_edge: edge does not exist"
    );

    EdgeIter prev_edge = m_prev_in_adjacency_list.get_label(from_id, to_id, edge_id);
    EdgeIter next_edge = m_next_in_adjacency_list.get_label(from_id, to_id, edge_id);

    if (prev_edge.id != edge_id)
        m_next_in_adjacency_list
            .update_label(from_id, prev_edge.neighbor_id, prev_edge.id, next_edge);

    if (next_edge.id != edge_id)
        m_prev_in_adjacency_list
            .update_label(from_id, next_edge.neighbor_id, next_edge.id, prev_edge);

    m_next_in_adjacency_list.erase_label(from_id, to_id, edge_id);
    m_prev_in_adjacency_list.erase_label(from_id, to_id, edge_id);

    adj.erase(it);
    m_number_of_edges--;
}

std::string Embedding::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Embedding:\n");
    for (const size_t node_id : get_nodes_ids()) {
        std::format_to(out, "{}: [ ", node_id);
        for (const size_t neighbor_id : get_neighbors(node_id))
            std::format_to(out, "{} ", neighbor_id);
        std::format_to(out, "]\n");
    }
    return result;
}

size_t Embedding::get_number_of_nodes() const { return m_adjacency_list.size(); }

size_t Embedding::get_number_of_edges() const { return m_number_of_edges; }

void Embedding::print() const { std::print("{}", to_string()); }

size_t compute_number_of_faces_in_embedding(const Embedding& embedding) {
    size_t number_of_faces = 0;
    OrientedEdgesContainer visited_edges;

    for (const size_t start_node : embedding.get_nodes_ids()) {
        if (embedding.get_degree_of_node(start_node) == 0) {
            ++number_of_faces;
            continue;
        }
        for (const EdgeIter start_edge : embedding.get_edges(start_node)) {
            if (visited_edges.has_edge(start_node, start_edge.neighbor_id, start_edge.id))
                continue;

            ++number_of_faces;
            size_t u = start_node;
            EdgeIter edge_uv = start_edge;

            while (!visited_edges.has_edge(u, edge_uv.neighbor_id, edge_uv.id)) {
                visited_edges.add_edge(u, edge_uv.neighbor_id, edge_uv.id);

                size_t v = edge_uv.neighbor_id;
                EdgeIter next_edge = embedding.next_in_adjacency_list(v, u, edge_uv.id);

                u = v;
                edge_uv = next_edge;
            }
        }
    }
    return number_of_faces;
}

std::vector<Path> compute_faces_in_embedding(const Graph& graph, const Embedding& embedding) {
    std::vector<Path> faces;
    OrientedEdgesContainer visited_edges;

    for (const size_t start_node : embedding.get_nodes_ids()) {
        for (const EdgeIter start_edge : embedding.get_edges(start_node)) {
            if (visited_edges.has_edge(start_node, start_edge.neighbor_id, start_edge.id))
                continue;

            faces.emplace_back();
            Path& current_face = faces.back();

            size_t u = start_node;
            EdgeIter edge_uv = start_edge;

            // traverse the boundary of the face
            while (!visited_edges.has_edge(u, edge_uv.neighbor_id, edge_uv.id)) {
                visited_edges.add_edge(u, edge_uv.neighbor_id, edge_uv.id);
                current_face.push_back(graph, u, edge_uv.id);

                size_t v = edge_uv.neighbor_id;
                EdgeIter next_edge = embedding.next_in_adjacency_list(v, u, edge_uv.id);

                u = v;
                edge_uv = next_edge;
            }
        }
    }

    return faces;
}

bool is_embedding_planar(const Embedding& embedding) {
    return compute_embedding_genus(embedding) == 0;
}

// This function verifies that for every edge from_id-to_id there is the edge to_id-from_id
// It is intended to be used only for debug purposes
bool Embedding::is_consistent() const {
    utilities::OrientedEdgesContainer edges;
    for (const size_t node_id : get_nodes_ids()) {
        for (const EdgeIter edge : get_edges(node_id)) {
            if (edges.has_edge(edge.neighbor_id, node_id, edge.id))
                edges.erase(edge.neighbor_id, node_id, edge.id);
            else
                edges.add_edge(node_id, edge.neighbor_id, edge.id);
        }
    }
    return edges.empty();
}

size_t compute_embedding_genus(
    size_t number_of_nodes,
    size_t number_of_edges,
    size_t number_of_faces,
    size_t connected_components
) {
    // f - e + v = 2(p - g)
    // f - e + v = 2p - 2g
    // 2g = 2p - f + e - v
    // g = p - (f - e + v) / 2
    const int n = static_cast<int>(number_of_nodes);
    const int e = static_cast<int>(number_of_edges);
    const int f = static_cast<int>(number_of_faces);
    const int p = static_cast<int>(connected_components);
    const int genus = p - (f - e + n) / 2;
    DOMUS_ASSERT(genus >= 0, "compute_embedding_genus: genus is negative");
    return static_cast<size_t>(genus);
}

size_t compute_embedding_genus(const Embedding& embedding) {
    DOMUS_ASSERT(
        embedding.is_consistent(),
        "compute_embedding_genus: embedding is not fully undirected"
    );
    size_t number_of_nodes = embedding.get_number_of_nodes();
    size_t number_of_edges = embedding.get_number_of_edges() / 2;
    size_t number_of_faces = compute_number_of_faces_in_embedding(embedding);
    size_t connected_components = algorithms::compute_number_of_connected_components(embedding);

    return compute_embedding_genus(
        number_of_nodes,
        number_of_edges,
        number_of_faces,
        connected_components
    );
}

std::vector<Embedding> compute_all_possible_embeddings(const Graph& graph) {
    std::vector<std::vector<std::vector<EdgeIter>>> all_permutations(graph.get_number_of_nodes());
    for (size_t u : graph.get_nodes_ids()) {
        auto edges_range = graph.get_edges(u);
        std::vector<EdgeIter> edges;
        for (const auto& edge : edges_range) {
            edges.push_back(edge);
        }

        if (edges.empty()) {
            all_permutations[u].push_back({});
            continue;
        }

        std::ranges::sort(edges, [](const EdgeIter& a, const EdgeIter& b) {
            if (a.neighbor_id != b.neighbor_id)
                return a.neighbor_id < b.neighbor_id;
            return a.id < b.id;
        });

        do {
            all_permutations[u].push_back(edges);
        } while (std::next_permutation(
            edges.begin() + 1,
            edges.end(),
            [](const EdgeIter& a, const EdgeIter& b) {
                if (a.neighbor_id != b.neighbor_id)
                    return a.neighbor_id < b.neighbor_id;
                return a.id < b.id;
            }
        ));
    }

    std::vector<Embedding> result;
    std::vector<size_t> current_choice(graph.get_number_of_nodes(), 0);

    auto construct_embedding = [&]() {
        Embedding emb(graph);
        for (size_t u = 0; u < graph.get_number_of_nodes(); ++u) {
            for (const auto& edge : all_permutations[u][current_choice[u]]) {
                emb.add_edge(u, edge.neighbor_id, edge.id);
            }
        }
        result.push_back(std::move(emb));
    };

    auto generate = [&](auto& self, size_t node_id) -> void {
        if (node_id == graph.get_number_of_nodes()) {
            construct_embedding();
            return;
        }
        for (size_t i = 0; i < all_permutations[node_id].size(); ++i) {
            current_choice[node_id] = i;
            self(self, node_id + 1);
        }
    };

    generate(generate, 0);

    return result;
}

static_assert(UndirectedGraphLike<Embedding>);

} // namespace domus::graph
