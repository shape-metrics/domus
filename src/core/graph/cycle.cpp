
#include "core/graph/cycle.hpp"

#include <algorithm>
#include <iostream>

Cycle::Cycle(const std::vector<int>& nodes_ids) : m_nodes_ids(nodes_ids) {}

void Cycle::clear() { m_nodes_ids.clear(); }

bool Cycle::empty() const { return m_nodes_ids.empty(); }

size_t Cycle::size() const { return m_nodes_ids.size(); }

void Cycle::insert(size_t index, int node_id) {
  m_nodes_ids.insert(m_nodes_ids.begin() + index, node_id);
}

void Cycle::remove_if_exists(int node_id) {
  auto it = std::find(m_nodes_ids.begin(), m_nodes_ids.end(), node_id);
  if (it != m_nodes_ids.end()) {
    m_nodes_ids.erase(it);
  }
}

void Cycle::add_in_between_if_exists(int node_id_1, int node_id_2,
                                     int in_between_node_id) {
  auto it_1 = std::find(m_nodes_ids.begin(), m_nodes_ids.end(), node_id_1);
  if (it_1 == m_nodes_ids.end()) return;
  int pos_1 = std::distance(m_nodes_ids.begin(), it_1);
  if (at(pos_1 + 1) == node_id_2)
    insert(pos_1, in_between_node_id);
  else if (at(pos_1 - 1) == node_id_2)
    insert(pos_1 - 1, in_between_node_id);
}

int Cycle::operator[](size_t index) const {
  return m_nodes_ids[index % size()];
}

int Cycle::at(size_t index) const { return m_nodes_ids.at(index % size()); }

std::vector<int>::const_iterator Cycle::begin() const {
  return m_nodes_ids.begin();
}

std::vector<int>::const_iterator Cycle::end() const {
  return m_nodes_ids.end();
}

std::string Cycle::to_string() const {
  std::string result = "Cycle: ";
  for (int id : m_nodes_ids) result += std::to_string(id) + " ";
  return result;
}

void Cycle::print() const { std::cout << to_string() << std::endl; }