#include "faces.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/path.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <print>
#include <vector>

namespace domus::torus {
using graph::Graph;
using graph::Path;

std::string face_type_to_string(FaceType face_type) {
    switch (face_type) {
    case FaceType::TYPE_1:
        return "Type 1";
    case FaceType::TYPE_2:
        return "Type 2";
    case FaceType::TYPE_3:
        return "Type 3";
    case FaceType::TYPE_4:
        return "Type 4";
    }
}

Face::Face(FaceType type, Path&& path, std::vector<Path>&& repeated_paths)
    : m_type(type), m_path(path), m_repeated_paths(repeated_paths) {}

FaceType Face::type() const { return m_type; }

const Path& Face::path() const { return m_path; }

const std::vector<Path>& Face::repeated_paths() const { return m_repeated_paths; }

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

std::string Face::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Face\n");
    std::format_to(out, "type: {}\n", face_type_to_string(type()));
    std::format_to(out, "{}", path().to_string());
    std::format_to(out, "Repeated paths:\n");
    for (const Path& path : repeated_paths())
        std::format_to(out, "{}", path.to_string());
    return result;
}

void Face::print() const { std::print("{}", to_string()); }

size_t node_id_count_in_path(const graph::Path& path, const size_t node_id) {
    size_t count = 0;
    for (size_t i = 0; i < path.number_of_edges(); ++i)
        if (path.node_id_at_position(i) == node_id)
            count++;
    return count;
}

Face compute_face_from_path(const graph::Path& path, const Graph& graph) {
    DOMUS_ASSERT(
        path.get_first_node_id() == path.get_last_node_id(),
        "compute_face_from_path: input path is not a cycle (so neither a face)"
    );
    DOMUS_ASSERT(path.number_of_edges() > 1, "compute_face_from_path: input path has only 1 edge");

    std::vector<std::pair<size_t, size_t>> edges_ids;
    for (size_t i = 0; i < path.number_of_edges(); i++)
        edges_ids.push_back({path.edge_id_at_position(i), i});
    std::sort(edges_ids.begin(), edges_ids.end(), [](auto a, auto b) { return a.first < b.first; });

    bool is_simple = true;
    for (size_t i = 0; i < edges_ids.size() - 1; i++)
        if (edges_ids[i].first == edges_ids[i + 1].first)
            is_simple = false;
    if (edges_ids.front() == edges_ids.back())
        is_simple = false;

    if (is_simple)
        return Face(FaceType::TYPE_1, Path(path), {});

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

    FaceType face_type;
    if (repeated_paths.size() == 1)
        face_type = FaceType::TYPE_2;
    else if (repeated_paths.size() == 2)
        face_type = FaceType::TYPE_3;
    else
        face_type = FaceType::TYPE_4;

    return Face(face_type, Path(path), std::move(repeated_paths));
}

} // namespace domus::torus