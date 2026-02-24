#ifndef MY_CYCLE_H
#define MY_CYCLE_H

#include <string>
#include <vector>

#include "domus/core/utils.hpp"

class Cycle {
    CircularSequence<int> m_nodes_ids;
    size_t next_index(size_t index) const;
    void reverse();

  public:
    explicit Cycle(std::ranges::input_range auto&& nodes_ids) : m_nodes_ids(nodes_ids) {}
    const auto& get_nodes_ids() const { return m_nodes_ids.get_elements(); }
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
    std::vector<int>::const_iterator begin() const;
    std::vector<int>::const_iterator end() const;
    std::string to_string() const;
    void print() const;
};

#endif
