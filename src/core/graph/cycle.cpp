
#include "domus/core/graph/cycle.hpp"

#include <cassert>
#include <format>
#include <print>

bool Cycle::empty() const { return m_nodes_ids.empty(); }

size_t Cycle::size() const { return m_nodes_ids.size(); }

void Cycle::insert(size_t index, size_t node_id) { m_nodes_ids.insert(index, node_id); }

void Cycle::append(size_t node_id) { m_nodes_ids.append(node_id); }

void Cycle::remove_if_exists(size_t node_id) { m_nodes_ids.remove_if_exists(node_id); }

void Cycle::add_in_between_if_exists(
    size_t node_id_1, size_t node_id_2, size_t in_between_node_id
) {
    assert(node_id_1 != node_id_2 && "Elements are equal");
    assert(!has_node(in_between_node_id) && "Element already exists");
    if (!has_node(node_id_1) || !has_node(node_id_2))
        return;
    if (next_of_node(node_id_1) == node_id_2)
        insert(node_position(node_id_1), in_between_node_id);
    else if (next_of_node(node_id_2) == node_id_1)
        insert(node_position(node_id_2), in_between_node_id);
}

size_t Cycle::operator[](size_t index) const { return m_nodes_ids[index]; }

size_t Cycle::at(size_t index) const { return m_nodes_ids.at(index); }

std::string Cycle::to_string() const { return std::format("Cycle"); }

void Cycle::print() const { std::println("{}", to_string()); }

size_t Cycle::prev_of_node(size_t node_id) const { return m_nodes_ids.prev_element(node_id); }

size_t Cycle::next_of_node(size_t node_id) const { return m_nodes_ids.next_element(node_id); }

bool Cycle::has_node(size_t node_id) const { return m_nodes_ids.has_element(node_id); }

size_t Cycle::node_position(size_t node_id) const { return *m_nodes_ids.element_position(node_id); }

void Cycle::for_each(std::function<void(size_t)> func) const { m_nodes_ids.for_each(func); }