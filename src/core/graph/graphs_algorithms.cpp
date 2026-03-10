#include "domus/core/graph/graphs_algorithms.hpp"

#include <cassert>
#include <functional>
#include <list>
#include <optional>
#include <print>
#include <queue>
#include <stack>

#include "domus/core/containers.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/tree/tree.hpp"
#include "domus/core/tree/tree_algorithms.hpp"

using namespace std;

bool is_graph_connected(const UndirectedGraph& graph) {
    if (graph.size() == 0)
        return true;
    NodesContainer visited;
    vector<int> stack;
    stack.push_back(graph.get_nodes_ids().get_one_node_id());
    while (!stack.empty()) {
        const int node_id = stack.back();
        stack.pop_back();
        visited.add_node(node_id);
        graph.get_neighbors_of_node(node_id).for_each([&stack, &visited](int neighbor_id) {
            if (!visited.has_node(neighbor_id))
                stack.push_back(neighbor_id);
        });
    }
    return visited.size() == graph.size();
}

vector<Cycle> compute_all_cycles_with_node_in_graph(
    const UndirectedGraph& graph, int node_id, const NodesContainer& taboo_nodes
) {
    vector<Cycle> cycles;
    NodesContainer visited;
    function<void(int, int, vector<int>&)> dfs =
        [&](const int current, const int start, vector<int>& path) {
            visited.add_node(current);
            path.push_back(current);
            graph.get_neighbors_of_node(current).for_each([&](int neighbor_id) {
                if (taboo_nodes.has_node(neighbor_id))
                    return;                                    // skip taboo nodes
                if (neighbor_id == start && path.size() > 2) { // found a cycle
                    cycles.emplace_back(path);
                } else if (!visited.has_node(neighbor_id)) {
                    dfs(neighbor_id, start, path);
                }
            });
            path.pop_back();
            visited.erase(current);
        };
    vector<int> path;
    dfs(node_id, node_id, path);
    return cycles;
}

vector<Cycle> compute_all_cycles_in_graph(const UndirectedGraph& graph) {
    vector<Cycle> all_cycles;
    NodesContainer taboo_nodes;
    graph.get_nodes_ids().for_each([&](int node_id) {
        vector<Cycle> cycles = compute_all_cycles_with_node_in_graph(graph, node_id, taboo_nodes);
        for (Cycle& cycle : cycles)
            all_cycles.push_back(cycle);
        taboo_nodes.add_node(node_id);
    });
    return all_cycles;
}

bool dfs_find_cycle(
    int node_id,
    const DirectedGraph& graph,
    Int_ToInt_HashMap& state,
    Int_ToInt_HashMap& parent,
    optional<int>& cycle_start,
    optional<int>& cycle_end
) {
    state.add(node_id, 1); // mark as visiting (gray)
    bool found_cycle = false;
    graph.get_out_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
        if (found_cycle)
            return;
        if (!state.has(neighbor_id)) { // unvisited
            parent.add(neighbor_id, node_id);
            found_cycle = dfs_find_cycle(neighbor_id, graph, state, parent, cycle_start, cycle_end);
        } else if (state.get(neighbor_id) == 1) {
            cycle_start = neighbor_id;
            cycle_end = node_id;
            found_cycle = true;
        }
    });
    state.update(node_id, 2); // mark as fully processed (black)
    return found_cycle;
}

optional<Cycle> find_a_cycle_in_graph(const DirectedGraph& graph) {
    Int_ToInt_HashMap state;
    Int_ToInt_HashMap parent;
    optional<int> cycle_start = std::nullopt;
    optional<int> cycle_end = std::nullopt;
    optional<Cycle> cycle;
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (cycle.has_value())
            return;
        if (!state.has(node_id))
            if (dfs_find_cycle(node_id, graph, state, parent, cycle_start, cycle_end)) {
                vector<int> cycle_vec;
                for (int v = cycle_end.value(); v != cycle_start; v = parent.get(v))
                    cycle_vec.push_back(v);
                cycle_vec.push_back(cycle_start.value());
                ranges::reverse(cycle_vec.begin(), cycle_vec.end());
                cycle.emplace(cycle_vec);
            }
    });
    return cycle;
}

vector<Cycle> compute_cycle_basis(const UndirectedGraph& graph) {
    assert(
        is_graph_connected(graph) && "Error in compute_cycle_basis: input graph is not connected"
    );
    const Tree spanning = *build_spanning_tree(graph);
    vector<Cycle> cycles;
    graph.get_nodes_ids().for_each([&](int node_id) {
        graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
            if (node_id > neighbor_id)
                return;
            if (spanning.has_edge(node_id, neighbor_id))
                return;
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
        });
    });
    return cycles;
}

optional<vector<int>> make_topological_ordering(const DirectedGraph& graph) {
    Int_ToInt_HashMap in_degree;
    graph.get_nodes_ids().for_each([&](int node_id) {
        in_degree[node_id] = static_cast<int>(graph.get_in_degree_of_node(node_id));
    });
    queue<int> queue;
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (in_degree[node_id] == 0)
            queue.push(node_id);
    });
    vector<int> topological_order;
    size_t count = 0;
    while (!queue.empty()) {
        int node_id = queue.front();
        ++count;
        queue.pop();
        topological_order.push_back(node_id);
        graph.get_out_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
            if (--in_degree[neighbor_id] == 0)
                queue.push(neighbor_id);
        });
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
    NodesContainer visited;
    vector<UndirectedGraph> components;
    function<void(int, UndirectedGraph& component)> explore_component =
        [&](int node_id, UndirectedGraph& component) {
            visited.add_node(node_id);
            graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
                if (!component.has_node(neighbor_id))
                    component.add_node(neighbor_id);
                if (!component.has_edge(node_id, neighbor_id))
                    component.add_edge(node_id, neighbor_id);
                if (!visited.has_node(neighbor_id)) {
                    explore_component(neighbor_id, component);
                }
            });
        };
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (!visited.has_node(node_id)) {
            components.emplace_back().add_node(node_id);
            explore_component(node_id, components.back());
        }
    });
    return components;
}

size_t compute_number_of_connected_components(const UndirectedGraph& graph) {
    NodesContainer visited;
    size_t components = 0;
    const function<void(int)> explore_component = [&](int start_node_id) {
        stack<int> stack;
        stack.push(start_node_id);
        while (!stack.empty()) {
            int node_id = stack.top();
            stack.pop();
            if (!visited.has_node(node_id)) {
                visited.add_node(node_id);
                graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
                    if (!visited.has_node(neighbor_id))
                        stack.push(neighbor_id);
                });
            }
        }
    };
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (!visited.has_node(node_id)) {
            components++;
            explore_component(node_id);
        }
    });
    return components;
}

void dfs_bic_com(
    const UndirectedGraph& graph,
    int node_id,
    Int_ToInt_HashMap& old_node_id_to_new_id,
    Int_ToInt_HashMap& prev_of_node,
    int& next_id_to_assign,
    Int_ToInt_HashMap& low_point,
    list<int>& stack_of_nodes,
    list<pair<int, int>>& stack_of_edges,
    vector<UndirectedGraph>& components,
    NodesContainer& cut_vertices
);

BiconnectedComponents compute_biconnected_components(const UndirectedGraph& graph) {
    Int_ToInt_HashMap old_node_id_to_new_id;
    Int_ToInt_HashMap prev_of_node;
    Int_ToInt_HashMap low_point;
    NodesContainer cut_vertices;
    vector<UndirectedGraph> components;
    int next_id_to_assign = 0;
    list<int> stack_of_nodes{};
    list<pair<int, int>> stack_of_edges{};
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (old_node_id_to_new_id.has(node_id)) // node visited
            return;
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
    });
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
    Int_ToInt_HashMap& old_node_id_to_new_id,
    Int_ToInt_HashMap& prev_of_node,
    int& next_id_to_assign,
    Int_ToInt_HashMap& low_point,
    list<int>& stack_of_nodes,
    list<pair<int, int>>& stack_of_edges,
    vector<UndirectedGraph>& components,
    NodesContainer& cut_vertices
) {
    old_node_id_to_new_id[node_id] = next_id_to_assign;
    low_point[node_id] = next_id_to_assign;
    ++next_id_to_assign;
    int children_number = 0;
    graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
        if (prev_of_node.has(node_id) && prev_of_node[node_id] == neighbor_id)
            return;
        if (!old_node_id_to_new_id.has(neighbor_id)) { // means the node is not visited
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
                if (prev_of_node.has(node_id)) // the root needs to be handled differently
                    // (handled at the end of the function)
                    cut_vertices.add_node(node_id);
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
    });
    if (!prev_of_node.has(node_id)) { // handling of node with no parents (the root)
        if (children_number >= 2)
            cut_vertices.add_node(node_id);
        else if (children_number == 0) { // node is isolated
            components.emplace_back().add_node(node_id);
        }
    }
}

string BiconnectedComponents::to_string() const {
    string result = "Biconnected Components:\n";
    result += "Cut vertices: ";
    m_cutvertices.for_each([&result](int cv) { result += std::to_string(cv) + " "; });
    result += "\nComponents:\n";
    for (const auto& component : m_components)
        result += component.to_string() + "\n";
    return result;
}

void BiconnectedComponents::print() const { println("{}", to_string()); }

bool bfs_bipartition(const UndirectedGraph& graph, int node_id, Bipartition& bipartition) {
    bipartition.set_side(node_id, false);
    queue<int> queue;
    queue.push(node_id);
    bool is_bipartite = true;
    while (!queue.empty()) {
        int current_id = queue.front();
        queue.pop();
        graph.get_neighbors_of_node(current_id).for_each([&](int neighbor_id) {
            if (!is_bipartite)
                return;
            if (!bipartition.has_node(neighbor_id)) {
                bipartition.set_side(neighbor_id, !bipartition.get_side(current_id));
                queue.push(neighbor_id);
            } else if (bipartition.are_in_same_side(neighbor_id, current_id))
                is_bipartite = false;
        });
    }
    return is_bipartite;
}

optional<Bipartition> compute_bipartition(const UndirectedGraph& graph) {
    Bipartition bipartition{};
    bool is_bipartite = true;
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (!is_bipartite)
            return;
        if (!bipartition.has_node(node_id))
            if (!bfs_bipartition(graph, node_id, bipartition))
                is_bipartite = false;
    });
    return bipartition;
}

optional<Cycle> find_a_cycle_in_graph(const UndirectedGraph& graph) {
    NodesContainer visited;
    Int_ToInt_HashMap parent;
    optional<Cycle> found_cycle;
    std::function<void(int, int)> dfs = [&](int node_id, int parent_id) {
        if (found_cycle)
            return;
        visited.add_node(node_id);
        parent.add(node_id, parent_id);
        graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
            if (found_cycle)
                return;
            if (neighbor_id == parent_id)
                return;
            if (visited.has_node(neighbor_id)) {
                // reconstruct cycle from u to v
                vector<int> cycle_vec;
                int curr = node_id;
                while (curr != neighbor_id) {
                    cycle_vec.push_back(curr);
                    curr = parent.get(curr);
                }
                cycle_vec.push_back(neighbor_id);
                found_cycle.emplace(cycle_vec);
                return;
            } else {
                dfs(neighbor_id, node_id);
            }
        });
    };
    graph.get_nodes_ids().for_each([&](int start_node_id) {
        if (!visited.has_node(start_node_id) && !found_cycle)
            dfs(start_node_id, -1);
    });
    return found_cycle;
}

const vector<UndirectedGraph>& BiconnectedComponents::get_components() const {
    return m_components;
}

BiconnectedComponents::BiconnectedComponents(
    NodesContainer&& cutvertices, vector<UndirectedGraph>&& components
)
    : m_cutvertices(std::move(cutvertices)), m_components(std::move(components)) {}

class IBipartitionImpl {
  public:
    Int_ToInt_HashMap partition_map{};
};

Bipartition::Bipartition() { m_impl = std::make_unique<IBipartitionImpl>(); }

Bipartition::~Bipartition() = default;
Bipartition::Bipartition(Bipartition&&) noexcept = default;
Bipartition& Bipartition::operator=(Bipartition&&) noexcept = default;

bool Bipartition::get_side(int node_id) const { return m_impl->partition_map.get(node_id); }

bool Bipartition::has_node(int node_id) const { return m_impl->partition_map.has(node_id); }

void Bipartition::set_side(int node_id, bool side) {
    assert(!m_impl->partition_map.has(node_id) && "Bipartition::set_side: node already exists");
    m_impl->partition_map[node_id] = side;
}

bool Bipartition::are_in_same_side(int node_id_1, int node_id_2) const {
    assert(
        has_node(node_id_1) && has_node(node_id_2) &&
        "Bipartition::are_in_same_side: nodes do not exist"
    );
    return get_side(node_id_1) == get_side(node_id_2);
}