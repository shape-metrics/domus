#include "domus/core/graph/graphs_algorithms.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <list>
#include <optional>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "domus/core/graph/graph.hpp"
#include "domus/core/tree/tree.hpp"
#include "domus/core/tree/tree_algorithms.hpp"

using namespace std;

bool is_graph_connected(const UndirectedGraph& graph) {
    if (graph.size() == 0)
        return true;
    unordered_set<int> visited;
    vector<int> stack;
    stack.push_back(*graph.get_nodes_ids().begin());
    while (!stack.empty()) {
        const int node_id = stack.back();
        stack.pop_back();
        visited.insert(node_id);
        for (int neighbor_id : graph.get_neighbors_of_node(node_id))
            if (!visited.contains(neighbor_id))
                stack.push_back(neighbor_id);
    }
    for (const int node_id : graph.get_nodes_ids())
        if (!visited.contains(node_id))
            return false;
    return true;
}

vector<Cycle> compute_all_cycles_with_node_in_graph(
    const UndirectedGraph& graph, int node_id, const unordered_set<int>& taboo_nodes
) {
    vector<Cycle> cycles;
    unordered_set<int> visited;
    function<void(int, int, vector<int>&)> dfs =
        [&](const int current, const int start, vector<int>& path) {
            visited.insert(current);
            path.push_back(current);
            for (int neighbor_id : graph.get_neighbors_of_node(current)) {
                if (taboo_nodes.contains(neighbor_id))
                    continue;                                  // skip taboo nodes
                if (neighbor_id == start && path.size() > 2) { // found a cycle
                    cycles.emplace_back(path);
                } else if (!visited.contains(neighbor_id)) {
                    dfs(neighbor_id, start, path);
                }
            }
            path.pop_back();
            visited.erase(current);
        };
    vector<int> path;
    dfs(node_id, node_id, path);
    return cycles;
}

vector<Cycle> compute_all_cycles_in_graph(const UndirectedGraph& graph) {
    vector<Cycle> all_cycles;
    unordered_set<int> taboo_nodes;
    for (int node_id : graph.get_nodes_ids()) {
        vector<Cycle> cycles = compute_all_cycles_with_node_in_graph(graph, node_id, taboo_nodes);
        for (Cycle& cycle : cycles)
            all_cycles.emplace_back(cycle.get_nodes_ids());
        taboo_nodes.insert(node_id);
    }
    return all_cycles;
}

bool dfs_find_cycle(
    int node_id,
    const DirectedGraph& graph,
    unordered_map<int, int>& state,
    unordered_map<int, int>& parent,
    optional<int>& cycle_start,
    optional<int>& cycle_end
) {
    state[node_id] = 1; // mark as visiting (gray)
    for (int neighbor_id : graph.get_out_neighbors_of_node(node_id)) {
        if (!state.contains(neighbor_id)) { // unvisited
            parent[neighbor_id] = node_id;
            if (dfs_find_cycle(neighbor_id, graph, state, parent, cycle_start, cycle_end))
                return true;
        } else if (state[neighbor_id] == 1) {
            cycle_start = neighbor_id;
            cycle_end = node_id;
            return true;
        }
    }
    state[node_id] = 2; // mark as fully processed (black)
    return false;
}

optional<Cycle> find_a_cycle_in_graph(const DirectedGraph& graph) {
    unordered_map<int, int> state;
    unordered_map<int, int> parent;
    optional<int> cycle_start = std::nullopt;
    optional<int> cycle_end = std::nullopt;
    for (int node_id : graph.get_nodes_ids())
        if (!state.contains(node_id))
            if (dfs_find_cycle(node_id, graph, state, parent, cycle_start, cycle_end))
                break;
    if (!cycle_start.has_value())
        return std::nullopt;
    vector<int> cycle;
    for (int v = cycle_end.value(); v != cycle_start; v = parent[v])
        cycle.push_back(v);
    cycle.push_back(cycle_start.value());
    ranges::reverse(cycle.begin(), cycle.end());
    return Cycle(cycle);
}

expected<vector<Cycle>, string> compute_cycle_basis(const UndirectedGraph& graph) {
    if (!is_graph_connected(graph))
        return std::unexpected("Error in compute_cycle_basis: input graph is not connected");
    const Tree spanning = *build_spanning_tree(graph);
    vector<Cycle> cycles;
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (node_id > neighbor_id)
                continue;
            if (spanning.has_edge(node_id, neighbor_id))
                continue;
            int common_ancestor = compute_common_ancestor(spanning, node_id, neighbor_id);
            vector<int> path1 = get_path_from_root(spanning, node_id);
            vector<int> path2 = get_path_from_root(spanning, neighbor_id);
            ranges::reverse(path1.begin(), path1.end());
            ranges::reverse(path2.begin(), path2.end());
            while (path1.back() != common_ancestor)
                path1.pop_back();
            while (path2.back() != common_ancestor)
                path2.pop_back();
            ranges::reverse(path1.begin(), path1.end());
            path1.insert(path1.end(), path2.begin(), path2.end());
            path1.pop_back();
            cycles.emplace_back(path1);
        }
    }
    return cycles;
}

optional<vector<int>> make_topological_ordering(const DirectedGraph& graph) {
    unordered_map<int, int> in_degree;
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_out_neighbors_of_node(node_id)) {
            if (!in_degree.contains(neighbor_id))
                in_degree[neighbor_id] = 0;
            in_degree[neighbor_id]++;
        }
    }
    queue<int> queue;
    vector<int> topological_order;
    for (int node_id : graph.get_nodes_ids())
        if (in_degree[node_id] == 0)
            queue.push(node_id);
    size_t count = 0;
    while (!queue.empty()) {
        int node_id = queue.front();
        ++count;
        queue.pop();
        topological_order.push_back(node_id);
        for (int neighbor_id : graph.get_out_neighbors_of_node(node_id)) {
            if (--in_degree[neighbor_id] == 0)
                queue.push(neighbor_id);
        }
    }
    if (count != graph.size())
        return std::nullopt; // graph contains a cycle
    return topological_order;
}

bool are_cycles_equivalent(const Cycle& cycle1, const Cycle& cycle2) {
    if (cycle1.size() != cycle2.size())
        return false;
    const int v = cycle1[0];
    if (!cycle2.has_node(v))
        return false;
    int current_1 = v;
    int current_2 = v;
    for (size_t i = 1; i < cycle1.size(); ++i) {
        current_1 = cycle1.next_of_node(current_1);
        current_2 = cycle2.next_of_node(current_2);
        if (current_1 != current_2)
            return false;
    }
    // Check the reverse
    current_1 = v;
    current_2 = v;
    for (size_t i = 1; i < cycle1.size(); ++i) {
        current_1 = cycle1.next_of_node(current_1);
        current_2 = cycle2.prev_of_node(current_2);
        if (current_1 != current_2)
            return false;
    }
    return true;
}

vector<UndirectedGraph> compute_connected_components(const UndirectedGraph& graph) {
    unordered_set<int> visited;
    vector<UndirectedGraph> components;
    function<void(int, UndirectedGraph& component)> explore_component =
        [&](int node_id, UndirectedGraph& component) {
            visited.insert(node_id);
            for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
                if (!component.has_node(neighbor_id))
                    component.add_node(neighbor_id);
                if (!component.has_edge(node_id, neighbor_id))
                    component.add_edge(node_id, neighbor_id);
                if (!visited.contains(neighbor_id)) {
                    explore_component(neighbor_id, component);
                }
            }
        };
    for (int node_id : graph.get_nodes_ids())
        if (!visited.contains(node_id)) {
            components.emplace_back().add_node(node_id);
            explore_component(node_id, components.back());
        }
    return components;
}

size_t compute_number_of_connected_components(const UndirectedGraph& graph) {
    unordered_set<int> visited;
    size_t components = 0;
    const function<void(int)> explore_component = [&](int start_node_id) {
        stack<int> stack;
        stack.push(start_node_id);
        while (!stack.empty()) {
            int node_id = stack.top();
            stack.pop();
            if (visited.insert(node_id).second)
                for (int neighbor_id : graph.get_neighbors_of_node(node_id))
                    if (!visited.contains(neighbor_id))
                        stack.push(neighbor_id);
        }
    };
    for (int node_id : graph.get_nodes_ids())
        if (!visited.contains(node_id)) {
            components++;
            explore_component(node_id);
        }
    return components;
}

void dfs_bic_com(
    const UndirectedGraph& graph,
    int node_id,
    unordered_map<int, int>& old_node_id_to_new_id,
    unordered_map<int, int>& prev_of_node,
    int& next_id_to_assign,
    unordered_map<int, int>& low_point,
    list<int>& stack_of_nodes,
    list<pair<int, int>>& stack_of_edges,
    vector<UndirectedGraph>& components,
    unordered_set<int>& cut_vertices
);

BiconnectedComponents compute_biconnected_components(const UndirectedGraph& graph) {
    unordered_map<int, int> old_node_id_to_new_id;
    unordered_map<int, int> prev_of_node;
    unordered_map<int, int> low_point;
    unordered_set<int> cut_vertices;
    vector<UndirectedGraph> components;
    int next_id_to_assign = 0;
    list<int> stack_of_nodes{};
    list<pair<int, int>> stack_of_edges{};
    for (int node_id : graph.get_nodes_ids())
        if (!old_node_id_to_new_id.contains(node_id)) // node not visited
            dfs_bic_com(
                graph,
                node_id,
                old_node_id_to_new_id,
                prev_of_node,
                next_id_to_assign,
                low_point,
                stack_of_nodes,
                stack_of_edges,
                components,
                cut_vertices
            );
    assert(
        stack_of_nodes.empty() && stack_of_edges.empty()
    ); // assessing algorithm finished correctly
    BiconnectedComponents result{std::move(cut_vertices), std::move(components)};
    return result;
}

void build_component(
    UndirectedGraph& component, const list<int>& nodes, const list<pair<int, int>>& edges
) {
    for (const int node : nodes)
        component.add_node(node);
    for (const auto& [from_id, to_id] : edges)
        component.add_edge(from_id, to_id);
}

void dfs_bic_com(
    const UndirectedGraph& graph,
    int node_id,
    unordered_map<int, int>& old_node_id_to_new_id,
    unordered_map<int, int>& prev_of_node,
    int& next_id_to_assign,
    unordered_map<int, int>& low_point,
    list<int>& stack_of_nodes,
    list<pair<int, int>>& stack_of_edges,
    vector<UndirectedGraph>& components,
    unordered_set<int>& cut_vertices
) {
    old_node_id_to_new_id[node_id] = next_id_to_assign;
    low_point[node_id] = next_id_to_assign;
    ++next_id_to_assign;
    int children_number = 0;
    for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
        if (prev_of_node.contains(node_id) && prev_of_node[node_id] == neighbor_id)
            continue;
        if (!old_node_id_to_new_id.contains(neighbor_id)) { // means the node is not visited
            list<int> new_stack_of_nodes{};
            list<pair<int, int>> new_stack_of_edges{};
            ++children_number;
            prev_of_node[neighbor_id] = node_id;
            new_stack_of_nodes.push_back(neighbor_id);
            new_stack_of_edges.emplace_back(node_id, neighbor_id);
            dfs_bic_com(
                graph,
                neighbor_id,
                old_node_id_to_new_id,
                prev_of_node,
                next_id_to_assign,
                low_point,
                new_stack_of_nodes,
                new_stack_of_edges,
                components,
                cut_vertices
            );
            if (low_point[neighbor_id] < low_point[node_id])
                low_point[node_id] = low_point[neighbor_id];
            if (low_point[neighbor_id] >= old_node_id_to_new_id[node_id]) {
                new_stack_of_nodes.push_back(node_id);
                components.emplace_back();
                build_component(components.back(), new_stack_of_nodes, new_stack_of_edges);
                if (prev_of_node.contains(node_id)) // the root needs to be handled differently
                    // (handled at the end of the function)
                    cut_vertices.insert(node_id);
            } else {
                stack_of_nodes.splice(stack_of_nodes.end(), new_stack_of_nodes);
                stack_of_edges.splice(stack_of_edges.end(), new_stack_of_edges);
            }
        } else { // node got already visited
            const int neighbor_node_id = old_node_id_to_new_id[neighbor_id];
            if (neighbor_node_id < old_node_id_to_new_id[node_id]) {
                stack_of_edges.emplace_back(node_id, neighbor_id);
                if (neighbor_node_id < low_point[node_id])
                    low_point[node_id] = neighbor_node_id;
            }
        }
    }
    if (!prev_of_node.contains(node_id)) { // handling of node with no parents (the root)
        if (children_number >= 2)
            cut_vertices.insert(node_id);
        else if (children_number == 0) { // node is isolated
            components.emplace_back().add_node(node_id);
        }
    }
}

string BiconnectedComponents::to_string() const {
    string result = "Biconnected Components:\n";
    result += "Cut vertices: ";
    for (const int cv : m_cutvertices)
        result += std::to_string(cv) + " ";
    result += "\nComponents:\n";
    for (const auto& component : m_components)
        result += component.to_string() + "\n";
    return result;
}

void BiconnectedComponents::print() const { std::cout << to_string() << std::endl; }

bool bfs_bipartition(
    const UndirectedGraph& graph, int node_id, unordered_map<int, bool>& bipartition
) {
    bipartition[node_id] = false;
    list<int> queue;
    queue.push_back(node_id);
    while (!queue.empty()) {
        int current_id = queue.front();
        queue.pop_front();
        for (int neighbor_id : graph.get_neighbors_of_node(current_id)) {
            if (!bipartition.contains(neighbor_id)) {
                bipartition[neighbor_id] = !bipartition[current_id];
                queue.push_back(neighbor_id);
            } else if (bipartition[neighbor_id] == bipartition[current_id])
                return false;
        }
    }
    return true;
}

optional<unordered_map<int, bool>> compute_bipartition(const UndirectedGraph& graph) {
    unordered_map<int, bool> bipartition{};
    for (int node_id : graph.get_nodes_ids())
        if (!bipartition.contains(node_id))
            if (!bfs_bipartition(graph, node_id, bipartition))
                return std::nullopt;
    return bipartition;
}

optional<Cycle> find_a_cycle_in_graph(const UndirectedGraph& graph) {
    if (graph.size() <= 2)
        return std::nullopt;
    unordered_set<int> visited;
    unordered_map<int, int> parent;
    for (int start_node_id : graph.get_nodes_ids()) {
        if (visited.contains(start_node_id))
            continue;
        vector<int> stack;
        stack.push_back(start_node_id);
        while (!stack.empty()) {
            int current_id = stack.back();
            stack.pop_back();
            visited.insert(current_id);
            for (int neighbor_id : graph.get_neighbors_of_node(current_id)) {
                if (!visited.contains(neighbor_id)) {
                    parent[neighbor_id] = current_id;
                    stack.push_back(neighbor_id);
                } else if (neighbor_id != parent[current_id]) {
                    vector<int> cycle;
                    int x = current_id;
                    int y = neighbor_id;
                    unordered_set<int> path_x;
                    while (true) {
                        path_x.insert(x);
                        if (!parent.contains(x))
                            break;
                        x = parent[x];
                    }
                    vector<int> path_to_lca;
                    while (!path_x.contains(y)) {
                        path_to_lca.push_back(y);
                        if (!parent.contains(y))
                            break;
                        y = parent[y];
                    }
                    cycle.push_back(y);
                    x = current_id;
                    while (x != y) {
                        cycle.push_back(x);
                        x = parent[x];
                    }
                    ranges::reverse(path_to_lca);
                    cycle.insert(cycle.end(), path_to_lca.begin(), path_to_lca.end());
                    return Cycle{cycle};
                }
            }
        }
    }
    return std::nullopt; // No cycle found
}

unordered_set<int>& BiconnectedComponents::get_cutvertices() { return m_cutvertices; }
const unordered_set<int>& BiconnectedComponents::get_cutvertices() const { return m_cutvertices; }

const vector<UndirectedGraph>& BiconnectedComponents::get_components() const {
    return m_components;
}

BiconnectedComponents::BiconnectedComponents(
    unordered_set<int>&& cutvertices, vector<UndirectedGraph>&& components
)
    : m_cutvertices(std::move(cutvertices)), m_components(std::move(components)) {}