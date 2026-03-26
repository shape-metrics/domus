#include "domus/core/graph/graphs_algorithms.hpp"

#include <algorithm>
#include <functional>
#include <optional>
#include <print>
#include <queue>
#include <stack>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/core/tree/tree_algorithms.hpp"

#include "../domus_debug.hpp"

namespace domus::graph::algorithms {
using namespace domus::graph;
using namespace domus::tree::algorithms;
using namespace domus::graph::utilities;
using namespace domus::tree;

bool is_graph_connected(const Graph& graph) {
    if (graph.get_number_of_nodes() <= 1)
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
    return visited.size() == graph.get_number_of_nodes();
}

bool dfs_find_cycle(
    size_t node_id,
    const Graph& graph,
    NodesLabels& state,
    NodesLabels& child_to_parent_edge,
    std::optional<size_t>& cycle_start,
    std::optional<size_t>& cycle_end,
    std::optional<size_t>& last_edge_id
) {
    DOMUS_ASSERT(state.get_label(node_id) == 0, "dfs_find_cycle: visiting already visited node");
    state.update_label(node_id, 1); // mark as visiting
    bool found_cycle = false;
    graph.for_each_out_edge(node_id, [&](size_t edge_id, size_t neighbor_id) {
        if (found_cycle)
            return;
        if (state.get_label(neighbor_id) == 0) { // unvisited
            child_to_parent_edge.add_label(neighbor_id, edge_id);
            found_cycle = dfs_find_cycle(
                neighbor_id,
                graph,
                state,
                child_to_parent_edge,
                cycle_start,
                cycle_end,
                last_edge_id
            );
        } else if (state.get_label(neighbor_id) == 1) { // found cycle
            cycle_start = neighbor_id;
            cycle_end = node_id;
            last_edge_id = edge_id;
            found_cycle = true;
        }
    });
    state.update_label(node_id, 2); // mark as fully processed
    return found_cycle;
}

std::optional<Cycle> find_a_directed_cycle_in_graph(const Graph& graph) {
    NodesLabels state(graph); // 0 means unvisited
    for (size_t node_id : graph.get_node_ids())
        state.add_label(node_id, 0u);
    NodesLabels child_to_parent_edge(graph);
    std::optional<size_t> cycle_start = std::nullopt;
    std::optional<size_t> cycle_end = std::nullopt;
    std::optional<size_t> last_edge_id = std::nullopt;
    std::optional<Cycle> cycle;
    for (size_t node_id : graph.get_node_ids()) {
        if (state.get_label(node_id) == 0)
            if (dfs_find_cycle(
                    node_id,
                    graph,
                    state,
                    child_to_parent_edge,
                    cycle_start,
                    cycle_end,
                    last_edge_id
                )) {
                size_t current_edge = child_to_parent_edge.get_label(*cycle_end);
                Path path;
                while (true) {
                    auto [prev_id, next_id] = graph.get_edge(current_edge);
                    path.push_front(graph, next_id, current_edge);
                    if (*cycle_start == prev_id)
                        break;
                    current_edge = child_to_parent_edge.get_label(prev_id);
                }
                path.push_back(graph, *cycle_end, *last_edge_id);
                cycle.emplace(path);
                return cycle;
            }
    }
    return cycle;
}

std::vector<Cycle> compute_cycle_basis(const Graph& graph) {
    DOMUS_ASSERT(is_graph_connected(graph), "compute_cycle_basis: input graph is not connected");

    const SpanningTree spanning_tree = *SpanningTree::compute(graph);
    const Tree& spanning = spanning_tree.get_tree();
    const NodesLabels& labels = spanning_tree.get_edge_ids();

    std::vector<Cycle> cycles;
    for (size_t node_id : graph.get_node_ids()) {
        for (auto [edge_id, neighbor_id] : graph.get_out_edges(node_id)) {
            if (spanning.has_edge(node_id, neighbor_id))
                continue;
            size_t common_ancestor = compute_common_ancestor(spanning, node_id, neighbor_id);
            std::vector<size_t> path1 = get_path_from_root(spanning, node_id);
            std::vector<size_t> path2 = get_path_from_root(spanning, neighbor_id);
            std::ranges::reverse(path1);
            std::ranges::reverse(path2);
            while (path1.back() != common_ancestor)
                path1.pop_back();
            while (path2.back() != common_ancestor)
                path2.pop_back();

            path1.pop_back();
            path2.pop_back();

            Path path;
            for (size_t n_id : path1)
                path.push_back(graph, n_id, labels.get_label(n_id));
            path.push_front(graph, node_id, edge_id);
            for (size_t n_id : path2)
                path.push_front(graph, n_id, labels.get_label(n_id));
            cycles.emplace_back(path);
        }
    }
    return cycles;
}

std::optional<std::vector<size_t>> make_topological_ordering(const Graph& graph) {
    NodesLabels in_degree(graph);
    for (size_t node_id : graph.get_node_ids())
        in_degree.add_label(node_id, graph.get_in_degree_of_node(node_id));
    std::queue<size_t> queue;
    for (size_t node_id : graph.get_node_ids())
        if (in_degree.get_label(node_id) == 0)
            queue.push(node_id);
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
    if (count != graph.get_number_of_nodes())
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
                explore_component(neighbor_id, component);
            }
            size_t new_neighbor_id = new_node_ids.get_label(neighbor_id);
            if (!component.are_neighbors(new_node_id, new_neighbor_id))
                component.add_edge(new_node_id, new_neighbor_id);
        });
    };
    for (size_t node_id : graph.get_node_ids()) {
        if (!visited.has_node(node_id)) {
            size_t new_node_id = components.emplace_back().add_node();
            new_node_ids.add_label(node_id, new_node_id);
            explore_component(node_id, components.back());
        }
    }
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
    for (size_t node_id : graph.get_node_ids()) {
        if (!visited.has_node(node_id)) {
            components++;
            explore_component(node_id);
        }
    }
    return components;
}

void dfs_bic_com(
    const Graph& graph,
    size_t node_id,
    NodesLabels& old_node_id_to_new_id,
    NodesLabels& prev_of_node,
    size_t& next_id_to_assign,
    NodesLabels& low_point,
    std::vector<Edge>& edge_stack,
    std::vector<Graph>& components,
    NodesContainer& cut_vertices,
    std::vector<NodesLabels>& components_to_old_nodes,
    NodesLabels& old_to_new_nodes
);

BiconnectedComponents BiconnectedComponents::compute(const Graph& graph) {
    NodesLabels old_node_id_to_new_id(graph);
    NodesLabels prev_of_node(graph);
    NodesLabels low_point(graph);
    NodesContainer is_cut_vertex(graph);
    std::vector<Graph> components;
    std::vector<NodesLabels> component_to_old_nodes;
    size_t next_id_to_assign = 0;
    std::vector<Edge> edge_stack{};
    NodesLabels old_to_new_nodes(graph);
    for (size_t node_id : graph.get_node_ids()) {
        old_to_new_nodes.add_label(node_id, graph.get_number_of_nodes());
    }
    for (size_t node_id : graph.get_node_ids()) {
        if (old_node_id_to_new_id.has_label(node_id)) // node visited
            continue;
        dfs_bic_com(
            graph,
            node_id,
            old_node_id_to_new_id,
            prev_of_node,
            next_id_to_assign,
            low_point,
            edge_stack,
            components,
            is_cut_vertex,
            component_to_old_nodes,
            old_to_new_nodes
        );
    }
    DOMUS_ASSERT(
        edge_stack.empty(),
        "compute_biconnected_components: some internal error took place"
    ); // assessing algorithm finished correctly
    std::vector<size_t> cut_vectices;
    for (size_t node_id : graph.get_node_ids()) {
        if (is_cut_vertex.has_node(node_id))
            cut_vectices.push_back(node_id);
    }
    BiconnectedComponents result{
        std::move(cut_vectices),
        std::move(components),
        std::move(component_to_old_nodes)
    };
    return result;
}

void build_component(
    const std::vector<Edge>& edges,
    std::vector<Graph>& components,
    std::vector<NodesLabels>& components_to_old_nodes,
    NodesLabels& old_to_new_nodes
) {
    // extracting unique nodes from the edges
    std::vector<size_t> nodes;
    for (const auto& [from_id, to_id] : edges) {
        nodes.push_back(from_id);
        nodes.push_back(to_id);
    }
    std::ranges::sort(nodes);
    auto ret = std::ranges::unique(nodes);
    nodes.erase(ret.begin(), ret.end());

    // building the component
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
    NodesLabels& old_node_id_to_new_id, // acts as discovery time (dfn)
    NodesLabels& prev_of_node,
    size_t& next_id_to_assign,
    NodesLabels& low_point,
    std::vector<Edge>& edge_stack,
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
        // ignore the edge back to our direct parent in the DFS tree
        if (prev_of_node.has_label(node_id) && prev_of_node.get_label(node_id) == neighbor_id) {
            return;
        }

        if (!old_node_id_to_new_id.has_label(neighbor_id)) { // unvisited
            ++children_number;
            prev_of_node.add_label(neighbor_id, node_id);
            edge_stack.push_back({node_id, neighbor_id});
            dfs_bic_com(
                graph,
                neighbor_id,
                old_node_id_to_new_id,
                prev_of_node,
                next_id_to_assign,
                low_point,
                edge_stack,
                components,
                cut_vertices,
                components_to_old_nodes,
                old_to_new_nodes
            );

            // update low point of the current node
            if (low_point.get_label(neighbor_id) < low_point.get_label(node_id)) {
                low_point.update_label(node_id, low_point.get_label(neighbor_id));
            }

            // if neighbor can't reach a node strictly prior to node_id
            if (low_point.get_label(neighbor_id) >= old_node_id_to_new_id.get_label(node_id)) {
                if (prev_of_node.has_label(node_id)) { // Not the root
                    if (!cut_vertices.has_node(node_id))
                        cut_vertices.add_node(node_id);
                }

                // extract the component: pop edges until we find the one we just traversed
                std::vector<Edge> comp_edges;
                bool done = false;
                while (!edge_stack.empty() && !done) {
                    Edge e = edge_stack.back();
                    edge_stack.pop_back();
                    comp_edges.push_back(e);

                    auto [u, v] = e;
                    if ((u == node_id && v == neighbor_id) || (u == neighbor_id && v == node_id)) {
                        done = true;
                    }
                }

                build_component(comp_edges, components, components_to_old_nodes, old_to_new_nodes);
            }
        } else { // visited (Back Edge)
            // only push back-edges going UP the DFS tree
            if (old_node_id_to_new_id.get_label(neighbor_id) <
                old_node_id_to_new_id.get_label(node_id)) {
                edge_stack.push_back({node_id, neighbor_id});
                if (old_node_id_to_new_id.get_label(neighbor_id) < low_point.get_label(node_id)) {
                    low_point.update_label(node_id, old_node_id_to_new_id.get_label(neighbor_id));
                }
            }
        }
    });

    // handle the root node separately
    if (!prev_of_node.has_label(node_id)) {
        if (children_number >= 2) {
            cut_vertices.add_node(node_id);
        } else if (children_number == 0) { // isolated node
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
    std::format_to(out, "\n");
    for (size_t i = 0; i < m_components.size(); ++i) {
        const Graph& component = m_components[i];
        const NodesLabels& labels = m_components_nodes_to_original_nodes[i];
        const std::string name = std::format("Component {}", i);
        std::format_to(out, "{}\n", component.to_string(true, labels, name));
    }
    return result;
}

void BiconnectedComponents::print() const { println("{}", to_string()); }

bool dfs_bipartition(const Graph& graph, size_t node_id, Bipartition& bipartition) {
    bipartition.set_side(node_id, false);
    std::stack<size_t> stack;
    stack.push(node_id);
    bool is_bipartite = true;
    while (!stack.empty() && is_bipartite) {
        size_t current_id = stack.top();
        stack.pop();
        graph.for_each_neighbor(current_id, [&](size_t neighbor_id) {
            if (!is_bipartite)
                return;
            if (!bipartition.has_node(neighbor_id)) {
                bipartition.set_side(neighbor_id, !bipartition.get_side(current_id));
                stack.push(neighbor_id);
            } else if (bipartition.are_in_same_side(neighbor_id, current_id))
                is_bipartite = false;
        });
    }
    return is_bipartite;
}

std::optional<Bipartition> Bipartition::compute(const Graph& graph) {
    Bipartition bipartition{graph};
    for (size_t node_id : graph.get_node_ids())
        if (!bipartition.has_node(node_id))
            if (!dfs_bipartition(graph, node_id, bipartition))
                return std::nullopt;
    return bipartition;
}

std::optional<Cycle> find_an_undirected_cycle_in_graph(const Graph& graph) {
    NodesContainer visited(graph);
    NodesLabels edge_to_parent(graph);
    std::optional<Cycle> found_cycle;
    std::function<void(size_t, int)> dfs = [&](size_t node_id, int parent_id) {
        if (found_cycle)
            return;
        visited.add_node(node_id);
        graph.for_each_edge(node_id, [&](size_t edge_id, size_t neighbor_id) {
            if (found_cycle)
                return;
            if (parent_id != -1 && neighbor_id == static_cast<size_t>(parent_id))
                return;
            if (visited.has_node(neighbor_id)) {
                size_t curr = node_id;
                Path path;
                while (curr != neighbor_id) {
                    size_t e_id = edge_to_parent.get_label(curr);
                    path.push_back(graph, curr, e_id);
                    auto [u, v] = graph.get_edge(e_id);
                    curr = (u == curr) ? v : u;
                }
                path.push_back(graph, neighbor_id, edge_id);
                found_cycle.emplace(path);
                return;
            } else {
                edge_to_parent.add_label(neighbor_id, edge_id);
                dfs(neighbor_id, static_cast<int>(node_id));
            }
        });
    };
    for (size_t start_node_id : graph.get_node_ids())
        if (!visited.has_node(start_node_id) && !found_cycle)
            dfs(start_node_id, -1);
    return found_cycle;
}

const std::vector<Graph>& BiconnectedComponents::get_components() const { return m_components; }

const NodesLabels& BiconnectedComponents::get_labels_of_component(size_t component_id) const {
    return m_components_nodes_to_original_nodes[component_id];
}

BiconnectedComponents::BiconnectedComponents(
    std::vector<size_t>&& cutvertices,
    std::vector<Graph>&& components,
    std::vector<NodesLabels>&& old_nodes
)
    : m_cutvertices{cutvertices}, m_components{components},
      m_components_nodes_to_original_nodes{old_nodes} {}

Bipartition::Bipartition(const Graph& graph) : m_size(graph.get_number_of_nodes()), m_side(graph) {}

bool Bipartition::get_side(size_t node_id) const {
    DOMUS_ASSERT(has_node(node_id), "Bipartition::get_side: node {} does not exist", node_id);
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

std::optional<SpanningTree> SpanningTree::compute(const Graph& graph) {
    if (graph.get_number_of_nodes() <= 1)
        return std::nullopt;
    NodesLabels edge_id_to_parent(graph);
    std::stack<size_t> stack;
    stack.push(0u);
    edge_id_to_parent.add_label(0, 0);
    size_t number_visited_nodes = 1;
    while (!stack.empty()) {
        size_t node_id = stack.top();
        stack.pop();
        graph.for_each_edge(node_id, [&](size_t edge_id, size_t neighbor_id) {
            if (!edge_id_to_parent.has_label(neighbor_id)) {
                edge_id_to_parent.add_label(neighbor_id, edge_id);
                ++number_visited_nodes;
                stack.push(neighbor_id);
            }
        });
    }
    if (number_visited_nodes != graph.get_number_of_nodes())
        return std::nullopt;
    Tree tree;
    for (size_t node_id = 1; node_id < graph.get_number_of_nodes(); ++node_id)
        tree.add_node();
    for (size_t node_id = 1; node_id < graph.get_number_of_nodes(); ++node_id) {
        size_t edge_id = edge_id_to_parent.get_label(node_id);
        auto [from_id, to_id] = graph.get_edge(edge_id);
        if (from_id == node_id)
            tree.set_parent(node_id, to_id);
        else
            tree.set_parent(node_id, from_id);
    }
    return SpanningTree(std::move(tree), std::move(edge_id_to_parent));
}

const Tree& SpanningTree::get_tree() const { return m_tree; }

const NodesLabels& SpanningTree::get_edge_ids() const { return m_edge_ids; }

SpanningTree::SpanningTree(const Tree&& tree, const NodesLabels&& edge_ids)
    : m_tree(tree), m_edge_ids(edge_ids) {}

bool is_cycle_in_graph(const Graph& graph, const Cycle& cycle) {
    for (size_t i = 0; i < cycle.size(); i++) {
        const size_t cycle_node_id = cycle.node_id_at(i);
        const size_t next_cycle_node_id = cycle.node_id_at(i + 1);
        const size_t edge_id = cycle.edge_id_at(i);
        if (!graph.are_neighbors(cycle_node_id, next_cycle_node_id))
            return false;
        const auto [from_id, to_id] = graph.get_edge(edge_id);
        if (from_id == cycle_node_id && to_id == next_cycle_node_id)
            continue;
        if (from_id == next_cycle_node_id && to_id == cycle_node_id)
            continue;
        return false;
    }
    return true;
}

bool do_cycles_intersect(const Cycle& cycle_1, const Cycle& cycle_2) {
    std::vector<size_t> nodes_in_cycle_1;
    std::vector<size_t> nodes_in_cycle_2;
    nodes_in_cycle_1.reserve(cycle_1.size());
    nodes_in_cycle_2.reserve(cycle_2.size());
    for (size_t node_id : cycle_1.get_nodes_ids())
        nodes_in_cycle_1.push_back(node_id);
    for (size_t node_id : cycle_2.get_nodes_ids())
        nodes_in_cycle_2.push_back(node_id);
    std::sort(nodes_in_cycle_1.begin(), nodes_in_cycle_1.end());
    std::sort(nodes_in_cycle_2.begin(), nodes_in_cycle_2.end());

    size_t i = 0, j = 0;
    while (i < nodes_in_cycle_1.size() && j < nodes_in_cycle_2.size()) {
        if (nodes_in_cycle_1[i] == nodes_in_cycle_2[j]) {
            return true;
        } else if (nodes_in_cycle_1[i] < nodes_in_cycle_2[j]) {
            i++;
        } else {
            j++;
        }
    }
    return false;
}

} // namespace domus::graph::algorithms