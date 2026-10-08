#include "fixed_rotation_planarity.hpp"

#include <algorithm>
#include <limits>
#include <span>
#include <vector>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/ogdf_utils.hpp"

namespace domus::torus::frp {

using namespace domus::graph;

using Adj = std::vector<std::vector<size_t>>;
constexpr size_t no_node = std::numeric_limits<size_t>::max();

namespace detail {

bool adjacent(const Adj& adj, size_t a, size_t b) {
    for (size_t x : adj[a]) {
        if (x == b)
            return true;
    }
    return false;
}

// Marks everything reachable from `start` in G minus the vertices b1, b2
// (pass no_node for "nothing removed"). `start` must not be b1 or b2.
void reach(const Adj& adj, size_t start, size_t b1, size_t b2, std::vector<char>& vis) {
    vis.assign(adj.size(), static_cast<char>(0));
    std::vector<size_t> stack{start};
    vis[start] = 1;
    while (!stack.empty()) {
        size_t x = stack.back();
        stack.pop_back();
        for (size_t y : adj[x]) {
            if (y == b1 || y == b2 || vis[y])
                continue;
            vis[y] = 1;
            stack.push_back(y);
        }
    }
}

// A concrete flip witness: remove `separator`, take the component `side`,
// and mirror that side. `side` contains no separator vertex.
struct FlipWitness {
    std::vector<size_t> separator;
    std::vector<char> side;
};

// Finds a vertex separator S of size <= 2 between s and t (s,t non-adjacent),
// using the final residual graph of the vertex-split flow network. For a
// disconnected graph S is empty and side is the component containing s.
bool find_separation_witness(const Adj& adj, size_t s, size_t t, FlipWitness& out) {
    if (adjacent(adj, s, t))
        return false;

    const size_t n = adj.size();
    const size_t num_flow_nodes = 2 * n;
    std::vector<size_t> to;
    std::vector<int> cap;
    std::vector<size_t> nxt;
    std::vector<size_t> head(num_flow_nodes, no_node);

    auto addEdge = [&](size_t a, size_t b, int c) {
        to.push_back(b);
        cap.push_back(c);
        nxt.push_back(head[a]);
        head[a] = to.size() - 1;
        to.push_back(a);
        cap.push_back(0);
        nxt.push_back(head[b]);
        head[b] = to.size() - 1;
    };

    constexpr int limit = 3;
    for (size_t w = 0; w < n; ++w) {
        addEdge(2 * w, 2 * w + 1, (w == s || w == t) ? limit : 1);
        for (size_t x : adj[w])
            addEdge(2 * w + 1, 2 * x, limit);
    }

    const size_t source = 2 * s + 1;
    const size_t sink = 2 * t;
    int flow = 0;
    std::vector<char> seen(num_flow_nodes);

    while (flow < limit) {
        std::fill(seen.begin(), seen.end(), static_cast<char>(0));
        std::vector<size_t> parent(num_flow_nodes, no_node);
        std::vector<size_t> queue{source};
        seen[source] = 1;

        for (size_t qi = 0; qi < queue.size() && !seen[sink]; ++qi) {
            size_t a = queue[qi];
            for (size_t e = head[a]; e != no_node; e = nxt[e]) {
                if (cap[e] > 0 && !seen[to[e]]) {
                    seen[to[e]] = 1;
                    parent[to[e]] = e;
                    queue.push_back(to[e]);
                }
            }
        }

        if (!seen[sink])
            break;

        for (size_t x = sink; x != source; x = to[parent[x] ^ 1]) {
            size_t p_edge = parent[x];
            cap[p_edge] -= 1;
            cap[p_edge ^ 1] += 1;
        }
        ++flow;
    }

    if (flow == limit)
        return false;

    // Since all non-vertex arcs have capacity 3, a cut of size < 3 can only
    // contain vertex-capacity edges in -> out.
    out.separator.clear();
    for (size_t w = 0; w < n; ++w) {
        if (w == s || w == t)
            continue;
        if (seen[2 * w] && !seen[2 * w + 1])
            out.separator.push_back(w);
    }
    DOMUS_ASSERT(out.separator.size() <= 2, "find_separation_witness: separator larger than 2");

    std::vector<char> removed(n, 0);
    for (size_t w : out.separator)
        removed[w] = 1;

    // Recover the actual component containing s after removing the separator.
    out.side.assign(n, 0);
    std::vector<size_t> stack{s};
    out.side[s] = 1;
    while (!stack.empty()) {
        size_t x = stack.back();
        stack.pop_back();
        for (size_t y : adj[x]) {
            if (removed[y] || out.side[y])
                continue;
            out.side[y] = 1;
            stack.push_back(y);
        }
    }

    DOMUS_ASSERT(
        !out.side[t],
        "find_separation_witness: extracted separator does not separate terminals"
    );

    return true;
}

// Is there t (or no t) such that mirroring the component C of G-{p,t} that
// contains q reverses q but not p?
//
// q lies inside the flipped bridge, so its local rotation is reversed. The
// boundary vertex p is reversed iff at least two of its incident edges lead
// into C (the edge p-t, when present, is not part of C).
//
// One DFS of G-p rooted at q gives all candidates in O(n). A final reach()
// reconstructs the witness side, still O(n).
bool component_flip(const Adj& adj, size_t p, size_t q, FlipWitness* witness = nullptr) {
    const size_t n = adj.size();
    std::vector<int> disc(n, -1), low(n, 0);
    std::vector<size_t> parent(n, no_node), it(n, 0);
    std::vector<int> sep(n, 0);
    int timer = 0;
    std::vector<size_t> stack{q};
    disc[q] = low[q] = timer++;

    while (!stack.empty()) { // iterative DFS in G-p
        size_t x = stack.back();
        if (it[x] < adj[x].size()) {
            size_t y = adj[x][it[x]++];
            if (y == p || y == parent[x])
                continue;
            if (disc[y] == -1) {
                parent[y] = x;
                disc[y] = low[y] = timer++;
                stack.push_back(y);
            } else {
                low[x] = std::min(low[x], disc[y]);
            }
        } else {
            stack.pop_back();
            if (parent[x] != no_node)
                low[parent[x]] = std::min(low[parent[x]], low[x]);
        }
    }

    int reachable = 0;
    for (size_t x : adj[p]) {
        if (disc[x] != -1)
            ++reachable;
    }

    // No t is needed: q lies in a component of G-p that touches p by at most
    // one edge, so flipping that component leaves p unchanged.
    if (reachable <= 1) {
        if (witness) {
            witness->separator = {p};
            reach(adj, q, p, no_node, witness->side);
        }
        return true;
    }

    // sep[t] = number of neighbours x of p that are separated from q by t
    // inside G-p, as detected by the usual low-point criterion.
    for (size_t x : adj[p]) {
        if (disc[x] == -1)
            continue;
        for (size_t c = x; parent[c] != no_node; c = parent[c]) {
            size_t t = parent[c];
            if (t != q && low[c] >= disc[t])
                ++sep[t];
        }
    }

    for (size_t t = 0; t < n; ++t) {
        if (t == q || disc[t] == -1)
            continue;
        const bool isNbr = adjacent(adj, p, t);
        if (reachable - (isNbr ? 1 : 0) - sep[t] <= 1) {
            if (witness) {
                witness->separator = {p, t};
                reach(adj, q, p, t, witness->side);
            }
            return true;
        }
    }
    return false;
}

bool get_toggle_witness(const Adj& adj, size_t u, size_t v, FlipWitness& witness) {
    if (!adjacent(adj, u, v)) {
        // If u and v are separated by <=2 vertices, flipping the side
        // containing u changes u's orientation and not v's.
        if (find_separation_witness(adj, u, v, witness))
            return true;
    }

    // For adjacent u,v a separator disjoint from {u,v} cannot separate them,
    // so only a flip whose separator contains one of them can change the
    // relative parity. The same test also handles cut-vertex cases and
    // disconnected graphs.
    if (component_flip(adj, u, v, &witness))
        return true;
    if (component_flip(adj, v, u, &witness))
        return true;

    return false;
}

void apply_flip(Embedding& embedding, const FlipWitness& witness) {
    const size_t n = embedding.get_number_of_nodes();
    std::vector<char> is_separator(n, 0);
    for (size_t x : witness.separator)
        is_separator[x] = 1;

    // Reverse the rotation at every internal vertex of the flipped bridge.
    // (The separator vertices are boundary vertices and are handled below.)
    for (size_t v = 0; v < n; ++v) {
        if (witness.side[v] && !is_separator[v])
            embedding.reverse_circular_order(v);
    }

    // At a boundary vertex, reverse the cyclic order if at least 2 incident
    // edges lead into the flipped side.
    for (size_t p : witness.separator) {
        size_t inside = 0;
        for (const auto edge : embedding.get_edges(p))
            if (witness.side[edge.neighbor_id])
                ++inside;

        if (inside >= 2)
            embedding.reverse_circular_order(p);
    }
}

int rotation_parity(const Embedding& embedding, size_t w, std::span<const size_t> rot) {
    const size_t deg = embedding.get_degree_of_node(w);
    if (deg <= 2)
        return 0;

    std::vector<size_t> emb_neighbors;
    emb_neighbors.reserve(deg);
    for (const auto edge : embedding.get_edges(w))
        emb_neighbors.push_back(edge.neighbor_id);

    DOMUS_ASSERT(
        emb_neighbors.size() == 3 && rot.size() == 3,
        "rotation_parity: expects degree 3 vertex"
    );

    int pos[3];
    for (size_t k = 0; k < 3; ++k) {
        auto it = std::find(rot.begin(), rot.end(), emb_neighbors[k]);
        DOMUS_ASSERT(
            it != rot.end(),
            "rotation_parity: embedding has neighbor not in prescribed rotation"
        );
        pos[k] = static_cast<int>(std::distance(rot.begin(), it));
    }

    return (pos[1] == (pos[0] + 1) % 3 && pos[2] == (pos[1] + 1) % 3) ? 0 : 1;
}

} // namespace detail

std::optional<Embedding> planar_with_fixed_rotations(
    const Graph& G, size_t u, std::span<const size_t> rot_u, size_t v, std::span<const size_t> rot_v
) {
    DOMUS_ASSERT(
        G.has_node(u) && G.has_node(v),
        "planar_with_fixed_rotations: u and v must belong to G"
    );
    DOMUS_ASSERT(u != v, "planar_with_fixed_rotations: u and v must be different vertices");

    const size_t n = G.get_number_of_nodes();
    Adj adj(n);
    for (size_t w = 0; w < n; ++w)
        for (size_t neighbor : G.get_neighbors(w))
            adj[w].push_back(neighbor);

    DOMUS_ASSERT(
        algorithms::is_graph_subcubic(G),
        "planar_with_fixed_rotations: graph must be subcubic (max degree 3)"
    );

    auto check_rotation = [&](size_t w, std::span<const size_t> rot) {
        DOMUS_ASSERT(
            rot.size() == G.get_degree_of_node(w),
            "planar_with_fixed_rotations: rotation must list all incident edges"
        );
        DOMUS_ASSERT(
            [&]() {
                for (size_t neighbor : G.get_neighbors(w))
                    if (std::find(rot.begin(), rot.end(), neighbor) == rot.end())
                        return false;
                return true;
            }(),
            "planar_with_fixed_rotations: rotation must list the neighbours of vertex"
        );
        DOMUS_ASSERT(
            [&]() {
                for (size_t i = 0; i < rot.size(); ++i) {
                    for (size_t j = i + 1; j < rot.size(); ++j) {
                        if (rot[i] == rot[j])
                            return false;
                    }
                }
                return true;
            }(),
            "planar_with_fixed_rotations: rotation must list distinct neighbours"
        );
    };

    check_rotation(u, rot_u);
    check_rotation(v, rot_v);

    auto opt_embedding = ogdf_utils::compute_planar_embedding(G);
    if (!opt_embedding.has_value())
        return std::nullopt;

    Embedding embedding = std::move(opt_embedding.value());

    const size_t deg_u = G.get_degree_of_node(u);
    const size_t deg_v = G.get_degree_of_node(v);

    int pu = detail::rotation_parity(embedding, u, rot_u);
    int pv = detail::rotation_parity(embedding, v, rot_v);

    // If at least one of u or v has degree <= 2, its rotation is symmetric.
    // If the other has parity 1, simply mirroring the whole embedding satisfies both.
    if (deg_u <= 2 || deg_v <= 2) {
        if (pu == 1 || pv == 1)
            embedding.reverse_all_circular_orders();
        return embedding;
    }

    // Both u and v have degree 3:
    if (pu == pv) {
        if (pu == 1)
            embedding.reverse_all_circular_orders();
        return embedding;
    }

    // Different parity: a single 1-/2-flip must toggle the relative parity.
    detail::FlipWitness witness;
    if (!detail::get_toggle_witness(adj, u, v, witness))
        return std::nullopt;

    detail::apply_flip(embedding, witness);

    pu = detail::rotation_parity(embedding, u, rot_u);
    pv = detail::rotation_parity(embedding, v, rot_v);
    DOMUS_ASSERT(pu == pv, "planar_with_fixed_rotations: flip did not toggle relative parity");
    if (pu == 1)
        embedding.reverse_all_circular_orders();

    DOMUS_ASSERT(
        graph::is_embedding_planar(embedding),
        "planar_with_fixed_rotations: constructed flip is not planar"
    );
    DOMUS_ASSERT(
        detail::rotation_parity(embedding, u, rot_u) == 0 &&
            detail::rotation_parity(embedding, v, rot_v) == 0,
        "planar_with_fixed_rotations: constructed embedding violates fixed rotations"
    );

    return embedding;
}

} // namespace domus::torus::frp
