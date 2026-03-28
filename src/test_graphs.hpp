#pragma once

#include <vector>

namespace domus::graph {
class Graph;
}

namespace domus::graph::test {
extern std::vector<Graph> forbidden_minors;
extern std::vector<Graph> two_cycle_graphs;
extern std::vector<Graph> planar_graphs;
} // namespace domus::graph::test