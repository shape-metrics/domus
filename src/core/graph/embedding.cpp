#include "domus/core/graph/embedding.hpp"

#include <cstddef>
#include <print>

#include "domus/core/graph/concept.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::graph {

Embedding::Embedding(const Graph& graph) {
    DOMUS_ASSERT(
        [](const Graph& graph) {
            std::vector<size_t> nodes;
            nodes.reserve(graph.get_number_of_nodes());
            for (size_t node_id : graph.get_nodes_ids())
                nodes.push_back(node_id);
            for (size_t node_id = 0; node_id < graph.get_number_of_nodes(); ++node_id)
                if (std::ranges::find(nodes, node_id) == nodes.end())
                    return false;
            return true;
        }(graph),
        "Embedding::Embedding: graph is not well formed"
    );
    for (size_t i = 0; i < graph.get_number_of_nodes(); i++)
        m_adjacency_list.push_back({});
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
    if (node_id < neighbor_id) {
        DOMUS_ASSERT(
            m_next_in_adjacency_list_1[edge_id].has_value(),
            "Embedding::next_in_adjacency_list: edge does not exist"
        );
        return *m_next_in_adjacency_list_1[edge_id];
    } else {
        DOMUS_ASSERT(
            m_next_in_adjacency_list_2[edge_id].has_value(),
            "Embedding::next_in_adjacency_list: edge does not exist"
        );
        return *m_next_in_adjacency_list_2[edge_id];
    }
}

size_t Embedding::get_degree_of_node(size_t node_id) const {
    return m_adjacency_list.at(node_id).size();
}

void Embedding::add_edge(size_t from_id, size_t to_id, size_t edge_id) {
    if (edge_id >= m_next_in_adjacency_list_1.size()) {
        m_next_in_adjacency_list_1.resize(edge_id + 1, std::nullopt);
        m_next_in_adjacency_list_2.resize(edge_id + 1, std::nullopt);
        m_prev_in_adjacency_list_1.resize(edge_id + 1, std::nullopt);
        m_prev_in_adjacency_list_2.resize(edge_id + 1, std::nullopt);
    }

    EdgeIter new_edge{edge_id, to_id};
    auto& adj = m_adjacency_list.at(from_id);

    if (adj.empty()) {
        if (from_id < to_id) {
            m_next_in_adjacency_list_1[edge_id] = new_edge;
            m_prev_in_adjacency_list_1[edge_id] = new_edge;
        } else {
            m_next_in_adjacency_list_2[edge_id] = new_edge;
            m_prev_in_adjacency_list_2[edge_id] = new_edge;
        }
    } else {
        auto [first_edge_id, first_to_id] = adj.front();
        auto [last_edge_id, last_to_id] = adj.back();

        // updating old prevs and nexts
        if (from_id < first_to_id)
            m_prev_in_adjacency_list_1[first_edge_id] = new_edge;
        else
            m_prev_in_adjacency_list_2[first_edge_id] = new_edge;

        if (from_id < last_to_id)
            m_next_in_adjacency_list_1[last_edge_id] = new_edge;
        else
            m_next_in_adjacency_list_2[last_edge_id] = new_edge;

        // adding prev and next of new edge
        if (from_id < to_id) {
            m_next_in_adjacency_list_1[edge_id] = {first_edge_id, first_to_id};
            m_prev_in_adjacency_list_1[edge_id] = {last_edge_id, last_to_id};
        } else {
            m_next_in_adjacency_list_2[edge_id] = {first_edge_id, first_to_id};
            m_prev_in_adjacency_list_2[edge_id] = {last_edge_id, last_to_id};
        }
    }

    adj.push_back(new_edge);
    m_number_of_edges++;
}

void Embedding::add_edge_after(size_t from_id, size_t to_id, size_t edge_id, size_t prev_edge_id) {
    DOMUS_ASSERT(from_id != to_id, "Embedding::add_edge_after: from_id and to_id are equal");
    DOMUS_ASSERT(
        has_node(from_id) && has_node(to_id),
        "Embedding::add_edge_after: node does not exist"
    );

    while (m_next_in_adjacency_list_1.size() <= edge_id) {
        m_next_in_adjacency_list_1.push_back(std::nullopt);
        m_next_in_adjacency_list_2.push_back(std::nullopt);
        m_prev_in_adjacency_list_1.push_back(std::nullopt);
        m_prev_in_adjacency_list_2.push_back(std::nullopt);
    }

    EdgeIter new_edge{edge_id, to_id};
    auto& adj = m_adjacency_list.at(from_id);

    // find the position of prev_edge_id in the adjacency list
    auto prev_it = std::ranges::find_if(adj, [prev_edge_id](const EdgeIter& e) {
        return e.id == prev_edge_id;
    });
    DOMUS_ASSERT(prev_it != adj.end(), "Embedding::add_edge_after: prev_edge_id not found");

    size_t prev_to_id = prev_it->neighbor_id;

    // get the next edge after prev_edge
    EdgeIter next_edge;
    if (from_id < prev_to_id)
        next_edge = *m_next_in_adjacency_list_1[prev_edge_id];
    else
        next_edge = *m_next_in_adjacency_list_2[prev_edge_id];

    size_t next_edge_id = next_edge.id;
    size_t next_to_id = next_edge.neighbor_id;

    // update prev's next to point to new edge
    if (from_id < prev_to_id)
        m_next_in_adjacency_list_1[prev_edge_id] = new_edge;
    else
        m_next_in_adjacency_list_2[prev_edge_id] = new_edge;

    // update next's prev to point to new edge
    if (from_id < next_to_id)
        m_prev_in_adjacency_list_1[next_edge_id] = new_edge;
    else
        m_prev_in_adjacency_list_2[next_edge_id] = new_edge;

    // set new edge's prev and next
    if (from_id < to_id) {
        m_prev_in_adjacency_list_1[edge_id] = {prev_edge_id, prev_to_id};
        m_next_in_adjacency_list_1[edge_id] = next_edge;
    } else {
        m_prev_in_adjacency_list_2[edge_id] = {prev_edge_id, prev_to_id};
        m_next_in_adjacency_list_2[edge_id] = next_edge;
    }

    // insert new edge after prev_it in the adjacency list
    adj.insert(prev_it + 1, new_edge);
    m_number_of_edges++;
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
    if (from_id < to_id) {
        EdgeIter prev_edge = *m_prev_in_adjacency_list_1[edge_id];
        EdgeIter next_edge = *m_next_in_adjacency_list_1[edge_id];

        if (prev_edge.id != edge_id) {
            if (from_id < prev_edge.neighbor_id)
                m_next_in_adjacency_list_1[prev_edge.id] = next_edge;
            else
                m_next_in_adjacency_list_2[prev_edge.id] = next_edge;
        }

        if (next_edge.id != edge_id) {
            if (from_id < next_edge.neighbor_id)
                m_prev_in_adjacency_list_1[next_edge.id] = prev_edge;
            else
                m_prev_in_adjacency_list_2[next_edge.id] = prev_edge;
        }

        m_next_in_adjacency_list_1[edge_id] = std::nullopt;
        m_prev_in_adjacency_list_1[edge_id] = std::nullopt;
    } else {
        EdgeIter prev_edge = *m_prev_in_adjacency_list_2[edge_id];
        EdgeIter next_edge = *m_next_in_adjacency_list_2[edge_id];

        if (prev_edge.id != edge_id) {
            if (from_id < prev_edge.neighbor_id)
                m_next_in_adjacency_list_1[prev_edge.id] = next_edge;
            else
                m_next_in_adjacency_list_2[prev_edge.id] = next_edge;
        }

        if (next_edge.id != edge_id) {
            if (from_id < next_edge.neighbor_id)
                m_prev_in_adjacency_list_1[next_edge.id] = prev_edge;
            else
                m_prev_in_adjacency_list_2[next_edge.id] = prev_edge;
        }

        m_next_in_adjacency_list_2[edge_id] = std::nullopt;
        m_prev_in_adjacency_list_2[edge_id] = std::nullopt;
    }

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
    utilities::OrientedEdgesContainer visited_edges(embedding);

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

std::vector<graph::Path>
compute_faces_in_embedding(const Graph& graph, const Embedding& embedding) {
    std::vector<graph::Path> faces;
    utilities::OrientedEdgesContainer visited_edges(graph);

    for (const size_t start_node : embedding.get_nodes_ids()) {
        for (const EdgeIter start_edge : embedding.get_edges(start_node)) {
            if (visited_edges.has_edge(start_node, start_edge.neighbor_id, start_edge.id))
                continue;

            graph::Path current_face;
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

            faces.push_back(std::move(current_face));
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
    utilities::OrientedEdgesContainer edges(get_number_of_edges());
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

static_assert(UndirectedGraphLike<Embedding>);

} // namespace domus::graph