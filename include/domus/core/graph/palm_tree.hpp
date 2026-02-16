#ifndef PALM_TREE_HPP
#define PALM_TREE_HPP

#include "domus/core/tree/tree.hpp"

class Graph;

class PalmTree {
    Tree m_tree;
};

// assumes the graph is biconnected
PalmTree compute_palm_tree(const Graph& graph);

#endif