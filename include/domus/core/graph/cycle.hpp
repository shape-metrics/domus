#ifndef MY_CYCLE_H
#define MY_CYCLE_H

#include <stddef.h>
#include <list>
#include <string>
#include <vector>

#include "domus/core/utils.hpp"

class Cycle {
    CircularSequence<int> m_nodes_ids;
    size_t next_index(size_t index) const;
    void reverse();

   public:
    explicit Cycle(const std::vector<int>& nodes_ids);
    explicit Cycle(const std::list<int>& nodes_ids);
    void clear();
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;
    void insert(size_t index, int node_id);
    void append(int node_id);
    void remove_if_exists(int node_id);
    void add_in_between_if_exists(
        int node_id_1,
        int node_id_2,
        int in_between_node_id);
    [[nodiscard]] int prev_of_node(int node_id) const;
    [[nodiscard]] int next_of_node(int node_id) const;
    [[nodiscard]] bool has_node(int node_id) const;
    [[nodiscard]] size_t node_position(int node_id) const;
    int operator[](size_t index) const;
    [[nodiscard]] int at(size_t index) const;
    [[nodiscard]] std::vector<int>::const_iterator begin() const;
    [[nodiscard]] std::vector<int>::const_iterator end() const;
    [[nodiscard]] std::string to_string() const;
    void print() const;
};

#endif
