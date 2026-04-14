#include "domus/core/graph/flow.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <queue>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/graph.hpp"

namespace domus::graph::flow {

struct Flow {
    size_t capacity;
    size_t flow;
};

std::vector<Path> max_vertex_disjoint_paths(const Graph& graph, size_t sink_id, size_t source_id) {
    DOMUS_ASSERT(
        graph.has_node(source_id) && graph.has_node(sink_id),
        "max_flow: source or sink not in graph"
    );
    DOMUS_ASSERT(source_id != sink_id, "max_flow: source and sink are the same");

    Graph flow_graph;
    for (size_t i = 0; i < graph.get_number_of_nodes(); i++) {
        flow_graph.add_node();
        flow_graph.add_node();
    }

    std::vector<Flow> edge_data;
    std::vector<size_t> rev_edge;

    auto add_flow_edge = [&](size_t u, size_t v, size_t cap) {
        size_t e1 = flow_graph.add_edge(u, v);
        size_t e2 = flow_graph.add_edge(v, u);

        if (edge_data.size() <= std::max(e1, e2)) {
            edge_data.resize(std::max(e1, e2) + 1, {0, 0});
            rev_edge.resize(std::max(e1, e2) + 1, 0);
        }
        edge_data[e1] = {cap, 0};
        edge_data[e2] = {0, 0};
        rev_edge[e1] = e2;
        rev_edge[e2] = e1;
        return e1;
    };

    auto get_res_cap = [&](size_t e_id) {
        size_t rev = rev_edge[e_id];
        return edge_data[e_id].capacity - edge_data[e_id].flow + edge_data[rev].flow;
    };

    auto push_flow = [&](size_t e_id, size_t amount) {
        size_t rev = rev_edge[e_id];
        size_t cancel = std::min(amount, edge_data[rev].flow);
        edge_data[rev].flow -= cancel;
        edge_data[e_id].flow += (amount - cancel);
    };

    const size_t INF = std::numeric_limits<size_t>::max() / 2;

    for (size_t i = 0; i < graph.get_number_of_nodes(); i++) {
        size_t capacity = (i == source_id || i == sink_id) ? INF : 1;
        add_flow_edge(2 * i, 2 * i + 1, capacity);
    }

    std::vector<std::optional<size_t>> flow_to_orig_edge;
    auto map_edge = [&](size_t flow_e, size_t orig_e) {
        if (flow_to_orig_edge.size() <= flow_e) {
            flow_to_orig_edge.resize(flow_e + 1, std::nullopt);
        }
        flow_to_orig_edge[flow_e] = orig_e;
    };

    for (const auto& edge : graph.get_all_edges()) {
        size_t u = edge.edge.from_id;
        size_t v = edge.edge.to_id;

        size_t u_out = 2 * u + 1;
        size_t v_in = 2 * v;

        size_t e1_fwd = add_flow_edge(u_out, v_in, 1);
        map_edge(e1_fwd, edge.id);

        size_t v_out = 2 * v + 1;
        size_t u_in = 2 * u;

        size_t e2_fwd = add_flow_edge(v_out, u_in, 1);
        map_edge(e2_fwd, edge.id);
    }

    size_t flow_source = 2 * source_id;
    size_t flow_sink = 2 * sink_id + 1;

    while (true) {
        std::vector<std::optional<size_t>> parent_edge(
            flow_graph.get_number_of_nodes(),
            std::nullopt
        );
        std::queue<size_t> q;

        q.push(flow_source);

        while (!q.empty()) {
            size_t u = q.front();
            q.pop();

            if (u == flow_sink)
                break;

            for (auto e_iter : flow_graph.get_out_edges(u)) {
                size_t e_id = e_iter.id;
                size_t v = e_iter.neighbor_id;

                if (!parent_edge[v].has_value() && v != flow_source && get_res_cap(e_id) > 0) {
                    parent_edge[v] = e_id;
                    q.push(v);
                }
            }
        }

        if (!parent_edge[flow_sink].has_value())
            break;

        size_t curr = flow_sink;
        size_t bottleneck = INF;
        while (curr != flow_source) {
            size_t e_id = parent_edge[curr].value();
            bottleneck = std::min(bottleneck, get_res_cap(e_id));
            curr = flow_graph.get_edge(e_id).from_id;
        }

        curr = flow_sink;
        while (curr != flow_source) {
            size_t e_id = parent_edge[curr].value();
            push_flow(e_id, bottleneck);
            curr = flow_graph.get_edge(e_id).from_id;
        }
    }

    std::vector<Path> paths;
    while (true) {
        std::vector<std::pair<size_t, size_t>> current_path;
        size_t curr_node = source_id;

        while (curr_node != sink_id) {
            bool found_next = false;
            size_t flow_node_out = 2 * curr_node + 1;

            for (auto e_iter : flow_graph.get_out_edges(flow_node_out)) {
                size_t edge_id = e_iter.id;

                if (edge_data.size() > edge_id && flow_to_orig_edge.size() > edge_id &&
                    flow_to_orig_edge[edge_id].has_value()) {
                    if (edge_data[edge_id].flow > 0) {
                        size_t orig_edge_id = flow_to_orig_edge[edge_id].value();
                        size_t next_node = e_iter.neighbor_id / 2;

                        edge_data[edge_id].flow--;
                        current_path.push_back({curr_node, orig_edge_id});
                        curr_node = next_node;
                        found_next = true;
                        break;
                    }
                }
            }
            if (!found_next)
                break;
        }

        if (curr_node == sink_id) {
            Path p;
            for (auto [prev_node, edge_id] : current_path) {
                p.push_back(graph, prev_node, edge_id);
            }
            paths.push_back(p);
        } else {
            break;
        }
    }

    return paths;
}

std::vector<Path> max_vertex_disjoint_cycles(const Graph& graph, size_t node_id) {
    DOMUS_ASSERT(graph.has_node(node_id), "get_vertex_disjoint_cycles: node not in graph");

    std::vector<Path> paths;
    std::vector<bool> used_node(graph.get_number_of_nodes(), false);
    used_node[node_id] = true;

    const size_t INF = std::numeric_limits<size_t>::max();

    while (true) {
        std::queue<size_t> q;
        std::vector<size_t> root(graph.get_number_of_nodes(), INF);
        std::vector<size_t> parent(graph.get_number_of_nodes(), INF);
        std::vector<size_t> parent_edge_orig(graph.get_number_of_nodes(), INF);

        for (auto e_iter : graph.get_edges(node_id)) {
            size_t n = e_iter.neighbor_id;
            if (!used_node[n]) {
                q.push(n);
                root[n] = n;
                parent[n] = node_id;
                parent_edge_orig[n] = e_iter.id;
            }
        }

        bool found = false;
        size_t collision_u = INF;
        size_t collision_v = INF;
        size_t collision_e = INF;

        while (!q.empty()) {
            size_t u = q.front();
            q.pop();

            for (auto e_iter : graph.get_edges(u)) {
                size_t v = e_iter.neighbor_id;
                if (used_node[v])
                    continue;

                if (root[v] == INF) {
                    root[v] = root[u];
                    parent[v] = u;
                    parent_edge_orig[v] = e_iter.id;
                    q.push(v);
                } else if (root[v] != root[u]) {
                    collision_u = u;
                    collision_v = v;
                    collision_e = e_iter.id;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }

        if (!found)
            break;

        std::vector<std::pair<size_t, size_t>> seq;
        std::vector<std::pair<size_t, size_t>> p_u;

        size_t curr = collision_u;
        while (curr != node_id) {
            p_u.push_back({parent[curr], parent_edge_orig[curr]});
            curr = parent[curr];
        }
        std::reverse(p_u.begin(), p_u.end());
        for (auto x : p_u)
            seq.push_back(x);

        seq.push_back({collision_u, collision_e});

        curr = collision_v;
        while (curr != node_id) {
            seq.push_back({curr, parent_edge_orig[curr]});
            curr = parent[curr];
        }

        Path p;
        for (auto [pn, eid] : seq) {
            p.push_back(graph, pn, eid);
        }

        for (size_t i = 1; i <= p.number_of_edges(); ++i) {
            size_t n = p.node_id_at_position(i);
            if (n != node_id)
                used_node[n] = true;
        }
        paths.push_back(p);
    }

    return paths;
}

} // namespace domus::graph::flow
