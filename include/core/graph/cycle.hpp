#ifndef MY_CYCLE_H
#define MY_CYCLE_H

#include <string>
#include <vector>

class Cycle {
 private:
  std::vector<int> m_nodes_ids;

 public:
  Cycle(const std::vector<int>& nodes_ids);
  void clear();
  bool empty() const;
  size_t size() const;
  void insert(size_t index, int node_id);
  void remove_if_exists(int node_id);
  void add_in_between_if_exists(int node_id_1, int node_id_2,
                                int in_between_node_id);
  int operator[](size_t index) const;
  int at(size_t index) const;
  std::vector<int>::const_iterator begin() const;
  std::vector<int>::const_iterator end() const;
  std::string to_string() const;
  void print() const;
};

#endif