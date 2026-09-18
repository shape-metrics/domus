#include "domus/force_layout/planarity_preserving_force_layout.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace domus::force_layout {

struct Vector2D {
    double x = 0.0;
    double y = 0.0;

    Vector2D operator+(const Vector2D& o) const { return {x + o.x, y + o.y}; }
    Vector2D operator-(const Vector2D& o) const { return {x - o.x, y - o.y}; }
    Vector2D operator*(double s) const { return {x * s, y * s}; }
    Vector2D& operator+=(const Vector2D& o) {
        x += o.x;
        y += o.y;
        return *this;
    }

    double norm() const { return std::sqrt(x * x + y * y); }
};

// 2D cross product: (B - A) x (C - A)
static double signed_triangle_area_x2(const Vector2D& a, const Vector2D& b, const Vector2D& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void planarity_preserving_force_layout(
    const graph::Graph& graph,
    graph::Attributes& attributes,
    const graph::Embedding& embedding,
    const graph::utilities::NodesContainer& fixed_nodes,
    const ForceLayoutConfig& config
) {
    std::vector<size_t> active_nodes;
    for (const size_t node_id : graph.get_nodes_ids()) {
        if (attributes.has_position(node_id) && graph.get_degree_of_node(node_id) > 0) {
            active_nodes.push_back(node_id);
        }
    }

    if (active_nodes.empty())
        return;

    // Find bounding box
    double min_x = attributes.get_position_x(active_nodes[0]);
    double max_x = min_x;
    double min_y = attributes.get_position_y(active_nodes[0]);
    double max_y = min_y;

    for (size_t node_id : active_nodes) {
        double px = attributes.get_position_x(node_id);
        double py = attributes.get_position_y(node_id);
        min_x = std::min(min_x, px);
        max_x = std::max(max_x, px);
        min_y = std::min(min_y, py);
        max_y = std::max(max_y, py);
    }

    double width = std::max(max_x - min_x, 1.0);
    double height = std::max(max_y - min_y, 1.0);
    double area = width * height;

    double k = config.k;
    if (k <= 0.0) {
        k = std::sqrt(area / static_cast<double>(active_nodes.size())) * 0.75;
    }
    double k2 = k * k;

    double step = config.initial_step * std::max(width, height);

    struct IncidentTriangle {
        size_t v1;
        size_t v2;
        int initial_sign;
        double min_area;
    };
    std::vector<std::vector<IncidentTriangle>> node_triangles(graph.get_number_of_nodes());

    for (size_t u : active_nodes) {
        if (fixed_nodes.has_node(u))
            continue;

        std::vector<size_t> neighbors;
        for (const auto edge : embedding.get_edges(u)) {
            neighbors.push_back(edge.neighbor_id);
        }

        if (neighbors.size() < 3)
            continue;

        Vector2D pu{attributes.get_position_x(u), attributes.get_position_y(u)};
        const size_t num_neighbors = neighbors.size();

        for (size_t i = 0; i < num_neighbors; ++i) {
            size_t v1 = neighbors[i];
            size_t v2 = neighbors[(i + 1) % num_neighbors];
            Vector2D pv1{attributes.get_position_x(v1), attributes.get_position_y(v1)};
            Vector2D pv2{attributes.get_position_x(v2), attributes.get_position_y(v2)};

            double initial_area = signed_triangle_area_x2(pu, pv1, pv2);
            int sign = (initial_area >= 0.0) ? 1 : -1;
            double min_area = std::max(std::abs(initial_area) * 0.15, 1e-4);
            node_triangles[u].push_back({v1, v2, sign, min_area});
        }
    }

    // Main simulation loop
    for (size_t iter = 0; iter < config.iterations; ++iter) {
        std::vector<Vector2D> forces(graph.get_number_of_nodes(), {0.0, 0.0});

        // 1. Repulsive forces between all active nodes
        for (size_t i = 0; i < active_nodes.size(); ++i) {
            size_t u = active_nodes[i];
            Vector2D pu{attributes.get_position_x(u), attributes.get_position_y(u)};

            for (size_t j = i + 1; j < active_nodes.size(); ++j) {
                size_t v = active_nodes[j];
                Vector2D pv{attributes.get_position_x(v), attributes.get_position_y(v)};

                Vector2D delta = pu - pv;
                double dist = delta.norm();
                if (dist < 1e-4) {
                    delta = {1e-3 * (1.0 + (u % 3)), 1e-3 * (1.0 + (v % 3))};
                    dist = delta.norm();
                }

                double f_rep = config.repulsion_strength * (k2 / (dist * dist));
                Vector2D rep_vec = delta * (f_rep / dist);

                if (!fixed_nodes.has_node(u))
                    forces[u] += rep_vec;
                if (!fixed_nodes.has_node(v))
                    forces[v] += (rep_vec * -1.0);
            }
        }

        // 2. Attractive forces along edges
        for (const auto edge : graph.get_all_edges()) {
            size_t u = edge.edge.from_id;
            size_t v = edge.edge.to_id;
            if (!attributes.has_position(u) || !attributes.has_position(v))
                continue;

            Vector2D pu{attributes.get_position_x(u), attributes.get_position_y(u)};
            Vector2D pv{attributes.get_position_x(v), attributes.get_position_y(v)};

            Vector2D delta = pv - pu;
            double dist = delta.norm();
            if (dist < 1e-6)
                continue;

            // Fruchterman-Reingold spring attraction
            double f_att = config.attraction_strength * ((dist * dist) / k);
            Vector2D att_vec = delta * (f_att / dist);

            if (!fixed_nodes.has_node(u))
                forces[u] += att_vec;
            if (!fixed_nodes.has_node(v))
                forces[v] += (att_vec * -1.0);
        }

        // 3. Apply moves with planarity-preserving backtracking
        for (size_t u : active_nodes) {
            if (fixed_nodes.has_node(u) || node_triangles[u].empty())
                continue;

            Vector2D force = forces[u];
            double force_norm = force.norm();
            if (force_norm < 1e-8)
                continue;

            // Cap displacement to current step size
            double displacement_len = std::min(step, force_norm);
            Vector2D displacement = force * (displacement_len / force_norm);

            Vector2D pu{attributes.get_position_x(u), attributes.get_position_y(u)};

            // Backtracking line search: check if pu + candidate_disp preserves all triangle orientations
            Vector2D candidate_disp = displacement;
            bool accepted = false;

            for (int bisection = 0; bisection < 12; ++bisection) {
                Vector2D p_cand = pu + candidate_disp;
                bool triangles_ok = true;

                for (const auto& tri : node_triangles[u]) {
                    Vector2D pv1{attributes.get_position_x(tri.v1), attributes.get_position_y(tri.v1)};
                    Vector2D pv2{attributes.get_position_x(tri.v2), attributes.get_position_y(tri.v2)};

                    double new_area_x2 = signed_triangle_area_x2(p_cand, pv1, pv2);
                    int new_sign = (new_area_x2 >= 0.0) ? 1 : -1;

                    // Must preserve orientation and minimum area
                    if (new_sign != tri.initial_sign ||
                        std::abs(new_area_x2) < tri.min_area) {
                        triangles_ok = false;
                        break;
                    }
                }

                if (triangles_ok) {
                    accepted = true;
                    break;
                }

                candidate_disp = candidate_disp * 0.5;
            }

            if (accepted) {
                Vector2D new_pos = pu + candidate_disp;
                attributes.change_position(u, new_pos.x, new_pos.y);
            }
        }

        // Cool down
        step *= config.cooling_factor;
    }
}

} // namespace domus::force_layout
