#include "domus/core/graph/graphs_algorithms.hpp"

#include <algorithm>
#include <functional>
#include <list>
#include <optional>
#include <print>
#include <queue>
#include <stack>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/tree/tree.hpp"
#include "domus/core/tree/tree_algorithms.hpp"

#include "../domus_assert.hpp"

bool is_graph_connected(const Graph& graph) {
    if (graph.size() <= 1)
        return true;
    NodesContainer visited(graph);
    std::stack<size_t> stack;
    stack.push(0u);
    while (!stack.empty()) {
        size_t node_id = stack.top();
        stack.pop();
        if (visited.has_node(node_id))
            continue;
        visited.add_node(node_id);
        graph.for_each_neighbor(node_id, [&stack, &visited](size_t neighbor_id) {
            if (!visited.has_node(neighbor_id))
                stack.push(neighbor_id);
        });
    }
    return visited.size() == graph.size();
}

bool dfs_find_cycle(
    size_t node_id,
    const Graph& graph,
    NodesLabels& state,
    NodesLabels& parent,
    std::optional<size_t>& cycle_start,
    std::optional<size_t>& cycle_end
) {
    DOMUS_ASSERT(state.get_label(node_id) == 0, "dfs_find_cycle: visiting already visited node");
    state.update_label(node_id, 1); // mark as visiting
    bool found_cycle = false;
    graph.for_each_out_neighbor(node_id, [&](size_t neighbor_id) {
        if (found_cycle)
            return;
        if (state.get_label(neighbor_id) == 0) { // unvisited
            parent.add_label(neighbor_id, node_id);
            found_cycle = dfs_find_cycle(neighbor_id, graph, state, parent, cycle_start, cycle_end);
        } else if (state.get_label(neighbor_id) == 1) {
            cycle_start = neighbor_id;
            cycle_end = node_id;
            found_cycle = true;
        }
    });
    state.update_label(node_id, 2); // mark as fully processed
    return found_cycle;
}

std::optional<Cycle> find_a_directed_cycle_in_graph(const Graph& graph) {
    NodesLabels state(graph); // 0 means unvisited
    graph.for_each_node([&state](size_t node_id) { state.add_label(node_id, 0u); });
    NodesLabels parent(graph);
    std::optional<size_t> cycle_start = std::nullopt;
    std::optional<size_t> cycle_end = std::nullopt;
    std::optional<Cycle> cycle;
    graph.for_each_node([&](size_t node_id) {
        if (cycle.has_value())
            return;
        if (state.get_label(node_id) == 0)
            if (dfs_find_cycle(node_id, graph, state, parent, cycle_start, cycle_end)) {
                std::vector<size_t> cycle_vec;
                for (size_t v = cycle_end.value(); v != cycle_start; v = parent.get_label(v))
                    cycle_vec.push_back(v);
                cycle_vec.push_back(cycle_start.value());
                std::ranges::reverse(cycle_vec);
                cycle.emplace(cycle_vec);
            }
    });
    return cycle;
}

std::vector<Cycle> compute_cycle_basis(const Graph& graph) {
    DOMUS_ASSERT(is_graph_connected(graph), "compute_cycle_basis: input graph is not connected");
    const Tree spanning = *Tree::build_spanning_tree(graph);
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
            std::ranges::reverse(path1);
            std::ranges::reverse(path2);
            while (path1.back() != common_ancestor)
                path1.pop_back();
            while (path2.back() != common_ancestor)
                path2.pop_back();
            std::ranges::reverse(path1);
            path1.insert(path1.end(), path2.begin(), path2.end());
            path1.pop_back();
            cycles.emplace_back(path1);
        });
    });
    return cycles;
}

std::optional<std::vector<size_t>> make_topological_ordering(const Graph& graph) {
    NodesLabels in_degree(graph);
    graph.for_each_node([&](size_t node_id) {
        in_degree.add_label(node_id, graph.get_in_degree_of_node(node_id));
    });
    std::queue<size_t> queue;
    graph.for_each_node([&](size_t node_id) {
        if (in_degree.get_label(node_id) == 0)
            queue.push(node_id);
    });
    std::vector<size_t> topological_order;
    size_t count = 0;
    while (!queue.empty()) {
        size_t node_id = queue.front();
        ++count;
        queue.pop();
        topological_order.push_back(node_id);
        graph.for_each_out_neighbor(node_id, [&](size_t neighbor_id) {
            in_degree.update_label(neighbor_id, in_degree.get_label(neighbor_id) - 1);
            if (in_degree.get_label(neighbor_id) == 0)
                queue.push(neighbor_id);
        });
    }
    if (count != graph.size())
        return std::nullopt; // graph contains a cycle
    return topological_order;
}

std::pair<std::vector<Graph>, NodesLabels> compute_connected_components(const Graph& graph) {
    NodesContainer visited(graph);
    NodesLabels new_node_ids(graph); // node_id in component to node_id of graph
    std::vector<Graph> components;
    std::function<void(size_t, Graph& component)> explore_component = [&](size_t node_id,
                                                                          Graph& component) {
        visited.add_node(node_id);
        size_t new_node_id = new_node_ids.get_label(node_id);
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (!visited.has_node(neighbor_id)) {
                visited.add_node(neighbor_id);
                size_t new_node = component.add_node();
                new_node_ids.add_label(neighbor_id, new_node);
            }
            size_t new_neighbor_id = new_node_ids.get_label(neighbor_id);
            if (!component.are_neighbors(new_node_id, new_neighbor_id))
                component.add_edge(new_node_id, new_neighbor_id);
            if (!visited.has_node(neighbor_id))
                explore_component(neighbor_id, component);
        });
    };
    graph.for_each_node([&](size_t node_id) {
        if (!visited.has_node(node_id)) {
            size_t new_node_id = components.emplace_back().add_node();
            new_node_ids.add_label(node_id, new_node_id);
            explore_component(node_id, components.back());
        }
    });
    return {std::move(components), std::move(new_node_ids)};
}

size_t compute_number_of_connected_components(const Graph& graph) {
    NodesContainer visited(graph);
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
    NodesLabels& old_node_id_to_new_id,
    NodesLabels& prev_of_node,
    size_t& next_id_to_assign,
    NodesLabels& low_point,
    std::list<size_t>& stack_of_nodes,
    std::list<Edge>& stack_of_edges,
    std::vector<Graph>& components,
    NodesContainer& cut_vertices,
    std::vector<NodesLabels>& components_to_old_nodes,
    NodesLabels& old_to_new_nodes
);

BiconnectedComponents compute_biconnected_components(const Graph& graph) {
    NodesLabels old_node_id_to_new_id(graph);
    NodesLabels prev_of_node(graph);
    NodesLabels low_point(graph);
    NodesContainer is_cut_vertex(graph);
    std::vector<Graph> components;
    std::vector<NodesLabels> component_to_old_nodes;
    size_t next_id_to_assign = 0;
    std::list<size_t> stack_of_nodes{};
    std::list<Edge> stack_of_edges{};
    NodesLabels old_to_new_nodes(graph);
    graph.for_each_node([&graph, &old_to_new_nodes](size_t node_id) {
        old_to_new_nodes.add_label(node_id, graph.size());
    });
    graph.for_each_node([&](size_t node_id) {
        if (old_node_id_to_new_id.has_label(node_id)) // node visited
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
            is_cut_vertex,
            component_to_old_nodes,
            old_to_new_nodes
        );
    });
    DOMUS_ASSERT(
        stack_of_nodes.empty() && stack_of_edges.empty(),
        "compute_biconnected_components: some internal error took place"
    ); // assessing algorithm finished correctly
    std::vector<size_t> cut_vectices;
    graph.for_each_node([&cut_vectices, &is_cut_vertex](size_t node_id) {
        if (is_cut_vertex.has_node(node_id))
            cut_vectices.push_back(node_id);
    });
    BiconnectedComponents result{
        std::move(cut_vectices),
        std::move(components),
        std::move(component_to_old_nodes)
    };
    return result;
}

void build_component(
    const std::list<size_t>& nodes,
    const std::list<Edge>& edges,
    std::vector<Graph>& components,
    std::vector<NodesLabels>& components_to_old_nodes,
    NodesLabels& old_to_new_nodes
) {
    Graph& component = components.emplace_back();
    for (size_t node_id : nodes) {
        size_t new_node_id = component.add_node();
        old_to_new_nodes.update_label(node_id, new_node_id);
    }
    components_to_old_nodes.emplace_back(component);
    NodesLabels& labels = components_to_old_nodes.back();
    for (size_t node_id : nodes)
        labels.add_label(old_to_new_nodes.get_label(node_id), node_id);
    for (const auto& [from_id, to_id] : edges) {
        size_t new_from_id = old_to_new_nodes.get_label(from_id);
        size_t new_to_id = old_to_new_nodes.get_label(to_id);
        component.add_edge(new_from_id, new_to_id);
    }
}

void dfs_bic_com(
    const Graph& graph,
    size_t node_id,
    NodesLabels& old_node_id_to_new_id,
    NodesLabels& prev_of_node,
    size_t& next_id_to_assign,
    NodesLabels& low_point,
    std::list<size_t>& stack_of_nodes,
    std::list<Edge>& stack_of_edges,
    std::vector<Graph>& components,
    NodesContainer& cut_vertices,
    std::vector<NodesLabels>& components_to_old_nodes,
    NodesLabels& old_to_new_nodes
) {
    old_node_id_to_new_id.add_label(node_id, next_id_to_assign);
    low_point.add_label(node_id, next_id_to_assign);
    ++next_id_to_assign;
    size_t children_number = 0;
    graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
        if (prev_of_node.has_label(node_id) && prev_of_node.get_label(node_id) == neighbor_id)
            return;
        if (!old_node_id_to_new_id.has_label(neighbor_id)) { // means the node is not visited
            std::list<size_t> new_stack_of_nodes{};
            std::list<Edge> new_stack_of_edges{};
            ++children_number;
            prev_of_node.add_label(neighbor_id, node_id);
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
                cut_vertices,
                components_to_old_nodes,
                old_to_new_nodes
            );
            if (low_point.get_label(neighbor_id) < low_point.get_label(node_id))
                low_point.update_label(node_id, low_point.get_label(neighbor_id));
            if (low_point.get_label(neighbor_id) >= old_node_id_to_new_id.get_label(node_id)) {
                new_stack_of_nodes.push_back(node_id);
                build_component(
                    new_stack_of_nodes,
                    new_stack_of_edges,
                    components,
                    components_to_old_nodes,
                    old_to_new_nodes
                );
                if (prev_of_node.has_label(node_id)) // the root needs to be handled differently
                    // (handled at the end of the function)
                    cut_vertices.add_node(node_id);
            } else {
                stack_of_nodes.splice(stack_of_nodes.end(), new_stack_of_nodes);
                stack_of_edges.splice(stack_of_edges.end(), new_stack_of_edges);
            }
        } else { // node got already visited
            const size_t neighbor_node_id = old_node_id_to_new_id.get_label(neighbor_id);
            if (neighbor_node_id < old_node_id_to_new_id.get_label(node_id)) {
                stack_of_edges.emplace_back(node_id, neighbor_id);
                if (neighbor_node_id < low_point.get_label(node_id))
                    low_point.update_label(node_id, neighbor_node_id);
            }
        }
    });
    if (!prev_of_node.has_label(node_id)) { // handling of node with no parents (the root)
        if (children_number >= 2)
            cut_vertices.add_node(node_id);
        else if (children_number == 0) { // node is isolated
            size_t new_node = components.emplace_back().add_node();
            components_to_old_nodes.emplace_back(components.back());
            components_to_old_nodes.back().add_label(new_node, node_id);
        }
    }
}

std::string BiconnectedComponents::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Biconnected Components:\n");
    std::format_to(out, "Cut vertices:");
    for (size_t cutvertex : m_cutvertices)
        std::format_to(out, " {}", cutvertex);
    std::format_to(out, "\nComponents:\n");
    for (const auto& component : m_components)
        std::format_to(out, "{}\n", component.to_string());
    return result;
}

void BiconnectedComponents::print() const { println("{}", to_string()); }

bool bfs_bipartition(const Graph& graph, size_t node_id, Bipartition& bipartition) {
    bipartition.set_side(node_id, false);
    std::queue<size_t> queue;
    queue.push(node_id);
    bool is_bipartite = true;
    while (!queue.empty() && is_bipartite) {
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
    Bipartition bipartition{graph};
    bool is_bipartite = true;
    graph.for_each_node([&](size_t node_id) {
        if (!is_bipartite)
            return;
        if (!bipartition.has_node(node_id))
            if (!bfs_bipartition(graph, node_id, bipartition))
                is_bipartite = false;
    });
    if (!is_bipartite)
        return std::nullopt;
    return bipartition;
}

std::optional<Cycle> find_an_undirected_cycle_in_graph(const Graph& graph) {
    NodesContainer visited(graph);
    NodesLabels parent(graph);
    std::optional<Cycle> found_cycle;
    std::function<void(size_t, int)> dfs = [&](size_t node_id, int parent_id) {
        if (found_cycle)
            return;
        visited.add_node(node_id);
        parent.add_label(node_id, static_cast<size_t>(parent_id));
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
                    curr = parent.get_label(curr);
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
    std::vector<size_t>&& cutvertices,
    std::vector<Graph>&& components,
    std::vector<NodesLabels>&& old_nodes
)
    : m_cutvertices{cutvertices}, m_components{components},
      m_components_nodes_to_original_nodes{old_nodes} {}

Bipartition::Bipartition(const Graph& graph) : m_size(graph.size()), m_side(graph) {}

bool Bipartition::get_side(size_t node_id) const {
    DOMUS_ASSERT(
        has_node(node_id),
        std::format("Bipartition::get_side: node {} does not exist", node_id)
    );
    return m_side.get_label(node_id);
}

bool Bipartition::has_node(size_t node_id) const { return m_side.has_label(node_id); }

void Bipartition::set_side(size_t node_id, bool side) {
    DOMUS_ASSERT(!m_side.has_label(node_id), "Bipartition::set_side: node already exists");
    m_side.add_label(node_id, side);
}

bool Bipartition::are_in_same_side(size_t node_id_1, size_t node_id_2) const {
    DOMUS_ASSERT(
        has_node(node_id_1) && has_node(node_id_2),
        "Bipartition::are_in_same_side: nodes do not exist"
    );
    return get_side(node_id_1) == get_side(node_id_2);
}

std::string Bipartition::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Bipartition:\n");
    for (size_t node_id = 0; node_id < m_size; ++node_id) {
        size_t side = m_side.get_label(node_id);
        std::format_to(out, "{}: {}\n", node_id, side ? "left" : "right");
    }
    return result;
}

void Bipartition::print() const { std::print("{}", to_string()); }
