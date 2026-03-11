#pragma once

#include <functional>
#include <string>

#include "domus/core/circular_sequence.hpp"

class Cycle {
    CircularSequence m_nodes_ids;
    size_t next_index(size_t index) const;
    void reverse();

  public:
    Cycle(std::ranges::input_range auto&& nodes_ids) : m_nodes_ids(nodes_ids) {}
    Cycle(const Cycle& other) {
        other.for_each([this](size_t node_id) { append(node_id); });
    };
    Cycle& operator=(const Cycle& other) {
        if (this != &other) {
            m_nodes_ids.clear();
            other.for_each([this](size_t node_id) { append(node_id); });
        }
        return *this;
    }
    void clear();
    bool empty() const;
    size_t size() const;
    void insert(size_t index, size_t node_id);
    void append(size_t node_id);
    void remove_if_exists(size_t node_id);
    void add_in_between_if_exists(size_t node_id_1, size_t node_id_2, size_t in_between_node_id);
    size_t prev_of_node(size_t node_id) const;
    size_t next_of_node(size_t node_id) const;
    bool has_node(size_t node_id) const;
    size_t node_position(size_t node_id) const;
    size_t operator[](size_t index) const;
    size_t at(size_t index) const;
    void for_each(std::function<void(size_t)> func) const;
    std::string to_string() const;
    void print() const;
};