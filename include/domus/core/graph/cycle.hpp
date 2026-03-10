#pragma once

#include <string>

#include "domus/core/utils.hpp"

class Cycle {
    CircularSequence<int> m_nodes_ids;
    size_t next_index(size_t index) const;
    void reverse();

  public:
    Cycle(std::ranges::input_range auto&& nodes_ids) : m_nodes_ids(nodes_ids) {}
    Cycle(const Cycle& other) {
        other.for_each([this](int node_id) { append(node_id); });
    };
    Cycle& operator=(const Cycle& other) {
        if (this != &other) {
            m_nodes_ids.clear();
            other.for_each([this](int node_id) { append(node_id); });
        }
        return *this;
    }
    void clear();
    bool empty() const;
    size_t size() const;
    void insert(size_t index, int node_id);
    void append(int node_id);
    void remove_if_exists(int node_id);
    void add_in_between_if_exists(int node_id_1, int node_id_2, int in_between_node_id);
    int prev_of_node(int node_id) const;
    int next_of_node(int node_id) const;
    bool has_node(int node_id) const;
    size_t node_position(int node_id) const;
    int operator[](size_t index) const;
    int at(size_t index) const;
    void for_each(std::function<void(int)> func) const;
    std::string to_string() const;
    void print() const;
};