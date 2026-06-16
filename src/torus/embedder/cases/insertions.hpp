#pragma once

#include <cstddef>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"

namespace domus::torus {

enum class InsertionType { AFTER, BEFORE };

struct Insertion {
    const size_t node_id;
    const InsertionType type;
    const size_t edge_id;
    const size_t edge_id_to_insert;
};

struct PathInsertions {
    const Insertion head_insertion;
    const Insertion tail_insertion;
};

inline void
make_insertion(const graph::Graph& graph, graph::Embedding& embedding, Insertion insertion) {
    const graph::Edge edge = graph.get_edge(insertion.edge_id_to_insert);
    const size_t from_id = insertion.node_id;
    const size_t to_id = (edge.from_id == from_id) ? edge.to_id : edge.from_id;
    if (insertion.type == InsertionType::AFTER)
        embedding.add_edge_after(from_id, to_id, insertion.edge_id_to_insert, insertion.edge_id);
    else
        embedding.add_edge_before(from_id, to_id, insertion.edge_id_to_insert, insertion.edge_id);
}

inline void
remove_insertion(const graph::Graph& graph, graph::Embedding& embedding, Insertion insertion) {
    const graph::Edge edge = graph.get_edge(insertion.edge_id_to_insert);
    const size_t from_id = insertion.node_id;
    const size_t to_id = (edge.from_id == from_id) ? edge.to_id : edge.from_id;
    embedding.remove_edge(from_id, to_id, insertion.edge_id_to_insert);
}

} // namespace domus::torus