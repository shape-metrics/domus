
#include "domus/core/graph/cycle.hpp"

#include <cassert>
#include <format>
#include <print>
#include <ranges>

using namespace std;

void Cycle::clear() { m_nodes_ids.clear(); }

bool Cycle::empty() const { return m_nodes_ids.empty(); }

size_t Cycle::size() const { return m_nodes_ids.size(); }

void Cycle::insert(const size_t index, const int node_id) { m_nodes_ids.insert(index, node_id); }

void Cycle::append(const int node_id) { m_nodes_ids.append(node_id); }

void Cycle::remove_if_exists(const int node_id) { m_nodes_ids.remove_if_exists(node_id); }

void Cycle::add_in_between_if_exists(
    const int node_id_1, const int node_id_2, const int in_between_node_id
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

int Cycle::operator[](const size_t index) const { return m_nodes_ids[index]; }

int Cycle::at(const size_t index) const { return m_nodes_ids.at(index); }

vector<int>::const_iterator Cycle::begin() const { return m_nodes_ids.begin(); }

vector<int>::const_iterator Cycle::end() const { return m_nodes_ids.end(); }

string Cycle::to_string() const {
    return std::format(
        "Cycle: {}",
        std::views::all(m_nodes_ids) | std::views::transform([](int id) {
            return std::to_string(id);
        }) | std::views::join_with(' ')
    );
}

void Cycle::print() const { std::println("{}", to_string()); }

int Cycle::prev_of_node(const int node_id) const { return m_nodes_ids.prev_element(node_id); }

int Cycle::next_of_node(const int node_id) const { return m_nodes_ids.next_element(node_id); }

bool Cycle::has_node(const int node_id) const { return m_nodes_ids.has_element(node_id); }

size_t Cycle::node_position(const int node_id) const {
    return *m_nodes_ids.element_position(node_id);
}

void Cycle::reverse() { m_nodes_ids.reverse(); }

size_t Cycle::next_index(const size_t index) const { return m_nodes_ids.next_index(index); }