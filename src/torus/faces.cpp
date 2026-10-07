#include "faces.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <vector>

#include "domus/core/debug.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/core/print.hpp"

namespace domus::torus {
using graph::Graph;
using graph::Path;
using namespace domus::graph::utilities;

std::string face_type_to_string(FaceType face_type) {
    switch (face_type) {
    case FaceType::TYPE_1:
        return "Type 1";
    case FaceType::TYPE_2:
        return "Type 2";
    case FaceType::TYPE_3:
        return "Type 3";
    }
    DOMUS_ASSERT(false, "face_type_to_string: invalid face type");
    return "";
}

Face::Face(
    const Graph& graph,
    FaceType type,
    Path&& path,
    std::vector<Path>&& repeated_paths,
    std::vector<Path>&& non_repeated_paths
)
    : m_type(type), m_path(std::move(path)), m_repeated_paths(std::move(repeated_paths)),
      m_non_repeated_paths(std::move(non_repeated_paths)) {
    for (const size_t node_id : graph.get_nodes_ids())
        m_is_node_in_repeated_path.add_label(node_id, {});
    if (m_repeated_paths.empty())
        return;
    for (size_t i = 0; i < m_repeated_paths.size(); i++) {
        const Path& repeated_path = m_repeated_paths[i];
        for (size_t j = 1; j < repeated_path.number_of_nodes() - 1; j++) {
            const size_t node_id = repeated_path.get_node_id_at_position(j);
            m_is_node_in_repeated_path.get_label(node_id).set(i);
        }
    }
    const size_t first_id = m_repeated_paths[0].get_first_node_id();
    const size_t last_id = m_repeated_paths[0].get_last_node_id();
    m_is_node_in_repeated_path.get_label(first_id).set();
    m_is_node_in_repeated_path.get_label(last_id).set();
}

FaceType Face::type() const { return m_type; }

const Path& Face::path() const { return m_path; }

const std::vector<Path>& Face::repeated_paths() const { return m_repeated_paths; }

const std::vector<Path>& Face::non_repeated_paths() const { return m_non_repeated_paths; }

const NodesLabels<std::bitset<8>>& Face::is_node_repeated_in_face() const {
    return m_is_node_in_repeated_path;
}

std::optional<size_t> is_face_simple(const Path& path) {
    std::vector<size_t> nodes_in_path;
    nodes_in_path.reserve(path.number_of_edges() + 1);
    for (auto [edge_id, prev_node_id] : path.get_edges())
        nodes_in_path.push_back(prev_node_id);

    std::sort(nodes_in_path.begin(), nodes_in_path.end());

    for (size_t i = 0; i < nodes_in_path.size() - 1; ++i)
        if (nodes_in_path[i] == nodes_in_path[i + 1])
            return nodes_in_path[i];

    return std::nullopt;
}

const std::string Face::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    domus::format_to(out, "Face {}\n", face_type_to_string(type()));
    domus::format_to(out, "{}\n", path().to_string());
    domus::format_to(out, "Repeated paths:\n");
    for (const Path& path : repeated_paths())
        domus::format_to(out, "{}\n", path.to_string());
    domus::format_to(out, "Non-repeated paths:\n");
    for (const Path& path : non_repeated_paths())
        domus::format_to(out, "{}\n", path.to_string());
    return result;
}

void Face::print() const { domus::print("{}", to_string()); }

size_t node_id_count_in_path(const Path& path, const size_t node_id) {
    size_t count = 0;
    for (size_t i = 0; i < path.number_of_edges(); ++i)
        if (path.get_node_id_at_position(i) == node_id)
            count++;
    return count;
}

Face compute_face_from_path(Path&& path, const Graph& graph) {
    DOMUS_ASSERT(
        path.get_first_node_id() == path.get_last_node_id(),
        "compute_face_from_path: input path is not a cycle"
    );
    DOMUS_ASSERT(path.number_of_edges() > 1, "compute_face_from_path: input path has only 1 edge");

    std::vector<std::pair<size_t, size_t>> edges_ids;
    edges_ids.reserve(path.number_of_edges());
    for (size_t i = 0; i < path.number_of_edges(); i++)
        edges_ids.push_back({path.get_edge_id_at_position(i), i});
    std::sort(edges_ids.begin(), edges_ids.end(), [](auto a, auto b) { return a.first < b.first; });

    bool is_simple = true;
    for (size_t i = 0; i < edges_ids.size() - 1; i++)
        if (edges_ids[i].first == edges_ids[i + 1].first)
            is_simple = false;
    if (edges_ids.front() == edges_ids.back())
        is_simple = false;

    if (is_simple) {
        std::vector<Path> non_repeated_paths;
        non_repeated_paths.push_back(path);
        return Face(graph, FaceType::TYPE_1, std::move(path), {}, std::move(non_repeated_paths));
    }

    std::vector<bool> did_handle_repeated_edge_at_position(path.number_of_edges(), false);

    std::vector<Path> repeated_paths;

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
        repeated_paths.emplace_back();
        Path& repeated_path = repeated_paths.back();
        while (path.get_edge_id_at_position(pos_1) == path.get_edge_id_at_position(pos_2)) {
            const size_t edge_id = path.get_edge_id_at_position(pos_1);
            const size_t node_id = path.get_node_id_at_position(pos_1);
            repeated_path.push_back(graph, node_id, edge_id);
            did_handle_repeated_edge_at_position[pos_1] = true;
            did_handle_repeated_edge_at_position[pos_2] = true;
            pos_1 = (pos_1 + 1) % path.number_of_edges();
            pos_2 = (pos_2 + path.number_of_edges() - 1) % path.number_of_edges();
        }

        pos_1 = (positions[0] + path.number_of_edges() - 1) % path.number_of_edges();
        pos_2 = (positions[1] + 1) % path.number_of_edges();
        while (path.get_edge_id_at_position(pos_1) == path.get_edge_id_at_position(pos_2)) {
            const size_t edge_id = path.get_edge_id_at_position(pos_1);
            const size_t node_id =
                path.get_node_id_at_position((pos_1 + 1) % path.number_of_edges());
            repeated_path.push_front(graph, node_id, edge_id);
            did_handle_repeated_edge_at_position[pos_1] = true;
            did_handle_repeated_edge_at_position[pos_2] = true;
            pos_1 = (pos_1 + path.number_of_edges() - 1) % path.number_of_edges();
            pos_2 = (pos_2 + 1) % path.number_of_edges();
        }
        edges_ids.pop_back();
        edges_ids.pop_back();
    }

    std::vector<Path> non_repeated_paths;
    std::optional<size_t> first_repeated_pos;
    for (size_t i = 0; i < path.number_of_edges(); i++) {
        if (did_handle_repeated_edge_at_position[i]) {
            first_repeated_pos = i;
            break;
        }
    }

    if (!first_repeated_pos.has_value()) {
        non_repeated_paths.push_back(path);
    } else {
        const size_t r = first_repeated_pos.value();
        const size_t n = path.number_of_edges();
        bool in_non_repeated = false;

        for (size_t k = 0; k < n; k++) {
            const size_t pos = (r + k) % n;
            if (!did_handle_repeated_edge_at_position[pos]) {
                if (!in_non_repeated) {
                    non_repeated_paths.emplace_back();
                    in_non_repeated = true;
                }
                non_repeated_paths.back().push_back(
                    graph,
                    path.get_node_id_at_position(pos),
                    path.get_edge_id_at_position(pos)
                );
            } else {
                in_non_repeated = false;
            }
        }
    }

    FaceType face_type;
    if (repeated_paths.size() == 1)
        face_type = FaceType::TYPE_2;
    else
        face_type = FaceType::TYPE_3;

    size_t total_edges_accounted = 0;
    for (const auto& rp : repeated_paths)
        total_edges_accounted += rp.number_of_edges() * 2;
    for (const auto& nrp : non_repeated_paths)
        total_edges_accounted += nrp.number_of_edges();
    DOMUS_ASSERT(
        total_edges_accounted == path.number_of_edges(),
        "compute_face_from_path: non-repeated paths and repeated paths do not partition face edges"
    );

    return Face(
        graph,
        face_type,
        std::move(path),
        std::move(repeated_paths),
        std::move(non_repeated_paths)
    );
}

} // namespace domus::torus
