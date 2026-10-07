#pragma once

#include <cstddef>

#include "domus/core/graph/graph_utilities.hpp"

#include "adjacencies.hpp"

namespace domus::torus {

// this class aims to provide an utility that is able to quickly (O(1) time) retrieve the position
// of NON repeated nodes in the faces. ONLY FOR NON REPEATED NODES.
// MOTIVATION:
// in one of the first steps to deal with type 2 faces, we need to verify that all the ordinary
// pieces are actually planarly embeddable in the embedding to extend. to do so, for each of these
// pieces, we need to pick one of the (at most 2) faces it is adjacent to. then we have to just try
// to embedd it together with that face. however if that face has many nodes that do not belong to
// the piece, it uselessly blows up the size of the thing we're checking for planarity. this gets
// done for all the pieces, resulting in much waste.
// what we can do, instead of building the graph G = (piece UNION face), we build
// G = (piece UNION contracted face), where contracted face is a cycle in which partecipate only the
// attachments of the particular piece, plus a separation node between consecutive ones (to avoid
// possible multiple edges in the graph). this way the size of G is O(k), where k is the number of
// nodes in the piece. to make this process efficient, we build and use this class
class NodesPositions {
    struct FacePosition {
        size_t face_index;
        size_t position_in_face;
    };
    graph::utilities::NodesLabels<std::array<std::optional<FacePosition>, 2>>
        m_node_position; // each node is in at most 3 faces, since the input graph is assumed to be
                         // cubic. however, since this class is meant to be interrogated only about
                         // attachment nodes, then we can store only 2 faces positions at most, for
                         // each node. if a node is an attachment of some piece, than it can be
                         // adjacent to at most 2 faces.

  public:
    NodesPositions(const Adjacencies& adjacencies);
    size_t get_position_of_node_in_face(size_t node_id, size_t face_id) const;
};
} // namespace domus::torus