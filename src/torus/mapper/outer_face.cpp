#include "outer_face.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/ogdf_utils.hpp"

namespace domus::torus::mapper {
using namespace graph::utilities;
using namespace domus::graph;

OuterFace compute_outer_face_from_path(Path&& path, const Graph& graph);

OuterFace compute_outer_face(const Graph& graph, const Embedding& embedding) {
    auto edges = ogdf_utils::find_kuratowski_subdivision(graph);

    NodesLabels<size_t> old_to_new_node;
    EdgesLabels<size_t> new_to_old_edge;

    Graph g;
    for (size_t edge_id : edges) {
        Edge edge = graph.get_edge(edge_id);
        if (!old_to_new_node.has_label(edge.from_id))
            old_to_new_node.add_label(edge.from_id, g.add_node());
        if (!old_to_new_node.has_label(edge.to_id))
            old_to_new_node.add_label(edge.to_id, g.add_node());
        size_t n_id_1 = old_to_new_node.get_label(edge.from_id);
        size_t n_id_2 = old_to_new_node.get_label(edge.to_id);
        new_to_old_edge.add_label(g.add_edge(n_id_1, n_id_2), edge_id);
    }

    g.print(true);

    auto basis = algorithms::compute_cycle_basis(g);

    std::vector<EdgesContainer> edges_in_cycles;
    for (size_t i = 0; i < basis.size(); i++) {
        Cycle& cycle = basis[i];
        edges_in_cycles.emplace_back();
        for (size_t j = 0; j < cycle.size(); j++)
            if (!edges_in_cycles.back().has_edge(new_to_old_edge.get_label(cycle.edge_id_at(j))))
                edges_in_cycles.back().add_edge(new_to_old_edge.get_label(cycle.edge_id_at(j)));
    }

    for (size_t i = 0; i < basis.size() - 1; i++) {
        for (size_t j = i + 1; j < basis.size(); j++) {
            Embedding e(embedding);
            for (const auto& edge : graph.get_all_edges()) {
                if (!edges_in_cycles[i].has_edge(edge.id) &&
                    !edges_in_cycles[j].has_edge(edge.id)) {
                    e.remove_edge(edge.edge.from_id, edge.edge.to_id, edge.id);
                    e.remove_edge(edge.edge.to_id, edge.edge.from_id, edge.id);
                }
            }
            auto faces = compute_faces_in_embedding(graph, e);
            if (faces.size() == 1)
                return compute_outer_face_from_path(Path(faces[0]), graph);
        }
    }
    DOMUS_ASSERT(false, "compute_outer_face: error");
    return compute_outer_face_from_path(Path{}, graph);
}

OuterFace::OuterFace(const Graph& graph, Path&& path, std::array<Path, 3>&& repeated_paths)
    : m_path(path), m_repeated_paths(repeated_paths) {
    if (m_repeated_paths[1].get_first_node_id() != m_repeated_paths[0].get_first_node_id())
        m_repeated_paths[1].reverse();
    if (m_repeated_paths[2].get_first_node_id() != m_repeated_paths[0].get_first_node_id())
        m_repeated_paths[2].reverse();
    DOMUS_ASSERT(
        m_repeated_paths[0].get_first_node_id() == m_repeated_paths[1].get_first_node_id() &&
            m_repeated_paths[0].get_first_node_id() == m_repeated_paths[2].get_first_node_id() &&
            m_repeated_paths[0].get_last_node_id() == m_repeated_paths[1].get_last_node_id() &&
            m_repeated_paths[0].get_last_node_id() == m_repeated_paths[2].get_last_node_id(),
        "OuterFace::OuterFace: endpoints of repeated paths do not match"
    );

    for (const size_t node_id : graph.get_nodes_ids())
        m_is_node_in_repeated_path.add_label(node_id, {});
    for (size_t i = 0; i < m_repeated_paths.size(); i++) {
        const Path& repeated_path = m_repeated_paths[i];
        for (size_t j = 1; j < repeated_path.number_of_nodes() - 1; j++) {
            const size_t node_id = repeated_path.node_id_at_position(j);
            m_is_node_in_repeated_path.get_label(node_id).set(i);
        }
    }
    const size_t first_id = m_repeated_paths[0].get_first_node_id();
    const size_t last_id = m_repeated_paths[0].get_last_node_id();
    m_is_node_in_repeated_path.get_label(first_id).set();
    m_is_node_in_repeated_path.get_label(last_id).set();
}

const Path& OuterFace::path() const { return m_path; }

const std::array<Path, 3>& OuterFace::repeated_paths() const { return m_repeated_paths; }

const graph::utilities::NodesLabels<std::bitset<3>>& OuterFace::is_node_in_repeated_path() const {
    return m_is_node_in_repeated_path;
}

std::string OuterFace::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "OuterFace\n");
    std::format_to(out, "{}", path().to_string());
    std::format_to(out, "Repeated paths:\n");
    for (const Path& path : repeated_paths())
        std::format_to(out, "{}", path.to_string());
    return result;
}

void OuterFace::print() const { std::print("{}", to_string()); }

OuterFace compute_outer_face_from_path(Path&& path, const Graph& graph) {
    DOMUS_ASSERT(
        path.get_first_node_id() == path.get_last_node_id(),
        "compute_face_from_path: input path is not a cycle"
    );
    DOMUS_ASSERT(path.number_of_edges() > 1, "compute_face_from_path: input path has only 1 edge");

    std::vector<std::pair<size_t, size_t>> edges_ids;
    edges_ids.reserve(path.number_of_edges());
    for (size_t i = 0; i < path.number_of_edges(); i++)
        edges_ids.push_back({path.edge_id_at_position(i), i});
    std::sort(edges_ids.begin(), edges_ids.end(), [](auto a, auto b) { return a.first < b.first; });

    std::vector<bool> did_handle_repeated_edge_at_position(path.number_of_edges(), false);

    std::array<Path, 3> repeated_paths;
    size_t next_index_repeated_path = 0;

    while (edges_ids.size() > 1) {
        size_t size = edges_ids.size();
        const size_t last = edges_ids[size - 1].first;
        const size_t prev = edges_ids[size - 2].first;
        if (last != prev) {
            edges_ids.pop_back();
            continue;
        }
        const std::array<size_t, 2> positions = {
            edges_ids[size - 1].second,
            edges_ids[size - 2].second
        };

        size_t pos_1 = positions[0];
        size_t pos_2 = positions[1];
        if (did_handle_repeated_edge_at_position[pos_1]) {
            edges_ids.pop_back();
            edges_ids.pop_back();
            continue;
        }
        Path& repeated_path = repeated_paths[next_index_repeated_path++];
        while (path.edge_id_at_position(pos_1) == path.edge_id_at_position(pos_2)) {
            const size_t edge_id = path.edge_id_at_position(pos_1);
            const size_t node_id = path.node_id_at_position(pos_1);
            repeated_path.push_back(graph, node_id, edge_id);
            did_handle_repeated_edge_at_position[pos_1] = true;
            did_handle_repeated_edge_at_position[pos_2] = true;
            pos_1 = (pos_1 + 1) % path.number_of_edges();
            pos_2 = (pos_2 + path.number_of_edges() - 1) % path.number_of_edges();
        }

        pos_1 = (positions[0] + path.number_of_edges() - 1) % path.number_of_edges();
        pos_2 = (positions[1] + 1) % path.number_of_edges();
        while (path.edge_id_at_position(pos_1) == path.edge_id_at_position(pos_2)) {
            const size_t edge_id = path.edge_id_at_position(pos_1);
            const size_t node_id = path.node_id_at_position((pos_1 + 1) % path.number_of_edges());
            repeated_path.push_front(graph, node_id, edge_id);
            did_handle_repeated_edge_at_position[pos_1] = true;
            did_handle_repeated_edge_at_position[pos_2] = true;
            pos_1 = (pos_1 + path.number_of_edges() - 1) % path.number_of_edges();
            pos_2 = (pos_2 + 1) % path.number_of_edges();
        }
        edges_ids.pop_back();
        edges_ids.pop_back();
    }

    return OuterFace(graph, std::move(path), std::move(repeated_paths));
}

} // namespace domus::torus::mapper