#pragma once

#include <vector>

#include "domus/core/graph/graph.hpp"

class Cycle;
class Segment;

Graph compute_interlacement_graph(const std::vector<Segment>& segments, const Cycle& cycle);