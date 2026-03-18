#pragma once

#include <functional>
#include <string>

#include "domus/core/circular_sequence.hpp"

class Cycle {
    CircularSequence m_nodes_ids;

  public:
    Cycle(std::ranges::input_range auto&& nodes_ids) : m_nodes_ids(nodes_ids) {}
    Cycle(const Cycle& other);
    Cycle& operator=(const Cycle& other);
    bool empty() const;
    size_t size() const;
    void insert(size_t index, size_t node_id);
    void add_in_between_if_exists(size_t node_id_1, size_t node_id_2, size_t in_between_node_id);
    bool has_node(size_t node_id) const;
    size_t node_position(size_t node_id) const;
    size_t operator[](size_t index) const;
    size_t at(size_t index) const;
    void for_each(std::function<void(size_t)> func) const;
    std::string to_string() const;
    void print() const;
};