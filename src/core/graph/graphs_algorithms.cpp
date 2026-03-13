#include "domus/core/graph/graphs_algorithms.hpp"

#include <algorithm>
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

#include "../domus_assert.hpp"

bool is_graph_connected(const Graph& graph) {
    if (graph.size() == 0)
        return true;
    NodesContainer visited;
    std::vector<size_t> stack;
    stack.push_back(graph.get_one_node_id());
    while (!stack.empty()) {
        const size_t node_id = stack.back();
        stack.pop_back();
        visited.add_node(node_id);
        graph.for_each_neighbor(node_id, [&stack, &visited](size_t neighbor_id) {
            if (!visited.has_node(neighbor_id))
                stack.push_back(neighbor_id);
        });
    }
    return visited.size() == graph.size();
}

bool dfs_find_cycle(
    size_t node_id,
    const Graph& graph,
    Int_ToInt_HashMap& state,
    Int_ToInt_HashMap& parent,
    std::optional<size_t>& cycle_start,
    std::optional<size_t>& cycle_end
) {
    state.add(node_id, 1); // mark as visiting (gray)
    bool found_cycle = false;
    graph.for_each_out_neighbors(node_id, [&](size_t neighbor_id) {
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

std::optional<Cycle> find_a_directed_cycle_in_graph(const Graph& graph) {
    Int_ToInt_HashMap state;
    Int_ToInt_HashMap parent;
    std::optional<size_t> cycle_start = std::nullopt;
    std::optional<size_t> cycle_end = std::nullopt;
    std::optional<Cycle> cycle;
    graph.for_each_node([&](size_t node_id) {
        if (cycle.has_value())
            return;
        if (!state.has(node_id))
            if (dfs_find_cycle(node_id, graph, state, parent, cycle_start, cycle_end)) {
                std::vector<size_t> cycle_vec;
                for (size_t v = cycle_end.value(); v != cycle_start; v = parent.get(v))
                    cycle_vec.push_back(v);
                cycle_vec.push_back(cycle_start.value());
                std::ranges::reverse(cycle_vec.begin(), cycle_vec.end());
                cycle.emplace(cycle_vec);
            }
    });
    return cycle;
}

std::vector<Cycle> compute_cycle_basis(const Graph& graph) {
    DOMUS_ASSERT(is_graph_connected(graph), "compute_cycle_basis: input graph is not connected");
    const Tree spanning = *build_spanning_tree(graph);
    std::vector<Cycle> cycles;
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (node_id > neighbor_id)
                return;
            if (spanning.has_edge(node_id, neighbor_id))
                return;
            size_t common_ancestor = compute_common_ancestor(spanning, node_id, neighbor_id);
            std::vector<size_t> path1 = get_path_from_root(spanning, node_id);
            std::vector<size_t> path2 = get_path_from_root(spanning, neighbor_id);
            std::ranges::reverse(path1.begin(), path1.end());
            std::ranges::reverse(path2.begin(), path2.end());
            while (path1.back() != common_ancestor)
                path1.pop_back();
            while (path2.back() != common_ancestor)
                path2.pop_back();
            std::ranges::reverse(path1.begin(), path1.end());
            path1.insert(path1.end(), path2.begin(), path2.end());
            path1.pop_back();
            cycles.emplace_back(path1);
        });
    });
    return cycles;
}

std::optional<std::vector<size_t>> make_topological_ordering(const Graph& graph) {
    Int_ToInt_HashMap in_degree;
    graph.for_each_node([&](size_t node_id) {
        in_degree[node_id] = graph.get_in_degree_of_node(node_id);
    });
    std::queue<size_t> queue;
    graph.for_each_node([&](size_t node_id) {
        if (in_degree[node_id] == 0)
            queue.push(node_id);
    });
    std::vector<size_t> topological_order;
    size_t count = 0;
    while (!queue.empty()) {
        size_t node_id = queue.front();
        ++count;
        queue.pop();
        topological_order.push_back(node_id);
        graph.for_each_out_neighbors(node_id, [&](size_t neighbor_id) {
            if (--in_degree[neighbor_id] == 0)
                queue.push(neighbor_id);
        });
    }
    if (count != graph.size())
        return std::nullopt; // graph contains a cycle
    return topological_order;
}

std::vector<Graph> compute_connected_components(const Graph& graph) {
    NodesContainer visited;
    std::vector<Graph> components;
    std::function<void(size_t, Graph& component)> explore_component = [&](size_t node_id,
                                                                          Graph& component) {
        visited.add_node(node_id);
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (!component.has_node(neighbor_id))
                component.add_node(neighbor_id);
            if (!component.are_neighbors(node_id, neighbor_id))
                component.add_edge(node_id, neighbor_id);
            if (!visited.has_node(neighbor_id)) {
                explore_component(neighbor_id, component);
            }
        });
    };
    graph.for_each_node([&](size_t node_id) {
        if (!visited.has_node(node_id)) {
            components.emplace_back().add_node(node_id);
            explore_component(node_id, components.back());
        }
    });
    return components;
}

size_t compute_number_of_connected_components(const Graph& graph) {
    NodesContainer visited;
    size_t components = 0;
    const std::function<void(size_t)> explore_component = [&](size_t start_node_id) {
        std::stack<size_t> stack;
        stack.push(start_node_id);
        while (!stack.empty()) {
            size_t node_id = stack.top();
            stack.pop();
            if (!visited.has_node(node_id)) {
                visited.add_node(node_id);
                graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
                    if (!visited.has_node(neighbor_id))
                        stack.push(neighbor_id);
                });
            }
        }
    };
    graph.for_each_node([&](size_t node_id) {
        if (!visited.has_node(node_id)) {
            components++;
            explore_component(node_id);
        }
    });
    return components;
}

void dfs_bic_com(
    const Graph& graph,
    size_t node_id,
    Int_ToInt_HashMap& old_node_id_to_new_id,
    Int_ToInt_HashMap& prev_of_node,
    size_t& next_id_to_assign,
    Int_ToInt_HashMap& low_point,
    std::list<size_t>& stack_of_nodes,
    std::list<Edge>& stack_of_edges,
    std::vector<Graph>& components,
    NodesContainer& cut_vertices
);

BiconnectedComponents compute_biconnected_components(const Graph& graph) {
    Int_ToInt_HashMap old_node_id_to_new_id;
    Int_ToInt_HashMap prev_of_node;
    Int_ToInt_HashMap low_point;
    NodesContainer cut_vertices;
    std::vector<Graph> components;
    size_t next_id_to_assign = 0;
    std::list<size_t> stack_of_nodes{};
    std::list<Edge> stack_of_edges{};
    graph.for_each_node([&](size_t node_id) {
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
    DOMUS_ASSERT(
        stack_of_nodes.empty() && stack_of_edges.empty(),
        "compute_biconnected_components: some internal error took place"
    ); // assessing algorithm finished correctly
    BiconnectedComponents result{std::move(cut_vertices), std::move(components)};
    return result;
}

void build_component(
    Graph& component, const std::list<size_t>& nodes, const std::list<Edge>& edges
) {
    for (const size_t node : nodes)
        component.add_node(node);
    for (const auto& [from_id, to_id] : edges)
        component.add_edge(from_id, to_id);
}

void dfs_bic_com(
    const Graph& graph,
    size_t node_id,
    Int_ToInt_HashMap& old_node_id_to_new_id,
    Int_ToInt_HashMap& prev_of_node,
    size_t& next_id_to_assign,
    Int_ToInt_HashMap& low_point,
    std::list<size_t>& stack_of_nodes,
    std::list<Edge>& stack_of_edges,
    std::vector<Graph>& components,
    NodesContainer& cut_vertices
) {
    old_node_id_to_new_id[node_id] = next_id_to_assign;
    low_point[node_id] = next_id_to_assign;
    ++next_id_to_assign;
    size_t children_number = 0;
    graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
        if (prev_of_node.has(node_id) && prev_of_node[node_id] == neighbor_id)
            return;
        if (!old_node_id_to_new_id.has(neighbor_id)) { // means the node is not visited
            std::list<size_t> new_stack_of_nodes{};
            std::list<Edge> new_stack_of_edges{};
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
            const size_t neighbor_node_id = old_node_id_to_new_id[neighbor_id];
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

std::string BiconnectedComponents::to_string() const {
    std::string result = "Biconnected Components:\n";
    result += "Cut vertices: ";
    m_cutvertices.for_each([&result](size_t cv) { result += std::to_string(cv) + " "; });
    result += "\nComponents:\n";
    for (const auto& component : m_components)
        result += component.to_string() + "\n";
    return result;
}

void BiconnectedComponents::print() const { println("{}", to_string()); }

bool bfs_bipartition(const Graph& graph, size_t node_id, Bipartition& bipartition) {
    bipartition.set_side(node_id, false);
    std::queue<size_t> queue;
    queue.push(node_id);
    bool is_bipartite = true;
    while (!queue.empty()) {
        size_t current_id = queue.front();
        queue.pop();
        graph.for_each_neighbor(current_id, [&](size_t neighbor_id) {
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

std::optional<Bipartition> compute_bipartition(const Graph& graph) {
    Bipartition bipartition{};
    bool is_bipartite = true;
    graph.for_each_node([&](size_t node_id) {
        if (!is_bipartite)
            return;
        if (!bipartition.has_node(node_id))
            if (!bfs_bipartition(graph, node_id, bipartition))
                is_bipartite = false;
    });
    return bipartition;
}

std::optional<Cycle> find_an_undirected_cycle_in_graph(const Graph& graph) {
    NodesContainer visited;
    Int_ToInt_HashMap parent;
    std::optional<Cycle> found_cycle;
    std::function<void(size_t, int)> dfs = [&](size_t node_id, int parent_id) {
        if (found_cycle)
            return;
        visited.add_node(node_id);
        parent.add(node_id, static_cast<size_t>(parent_id));
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (found_cycle)
                return;
            if (neighbor_id == static_cast<size_t>(parent_id))
                return;
            if (visited.has_node(neighbor_id)) {
                // reconstruct cycle from u to v
                std::vector<size_t> cycle_vec;
                size_t curr = node_id;
                while (curr != neighbor_id) {
                    cycle_vec.push_back(curr);
                    curr = parent.get(curr);
                }
                cycle_vec.push_back(neighbor_id);
                found_cycle.emplace(cycle_vec);
                return;
            } else {
                dfs(neighbor_id, static_cast<int>(node_id));
            }
        });
    };
    graph.for_each_node([&](size_t start_node_id) {
        if (!visited.has_node(start_node_id) && !found_cycle)
            dfs(start_node_id, -1);
    });
    return found_cycle;
}

const std::vector<Graph>& BiconnectedComponents::get_components() const { return m_components; }

BiconnectedComponents::BiconnectedComponents(
    NodesContainer&& cutvertices, std::vector<Graph>&& components
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

bool Bipartition::get_side(size_t node_id) const { return m_impl->partition_map.get(node_id); }

bool Bipartition::has_node(size_t node_id) const { return m_impl->partition_map.has(node_id); }

void Bipartition::set_side(size_t node_id, bool side) {
    DOMUS_ASSERT(!m_impl->partition_map.has(node_id), "Bipartition::set_side: node already exists");
    m_impl->partition_map[node_id] = side;
}

bool Bipartition::are_in_same_side(size_t node_id_1, size_t node_id_2) const {
    DOMUS_ASSERT(
        has_node(node_id_1) && has_node(node_id_2),
        "Bipartition::are_in_same_side: nodes do not exist"
    );
    return get_side(node_id_1) == get_side(node_id_2);
}