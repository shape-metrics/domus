
#include "domus/core/graph/cycle.hpp"

#include <format>
#include <print>

#include "../domus_debug.hpp"

Cycle::Cycle(const Cycle& other) {
    other.for_each([this](size_t node_id) { m_nodes_ids.append(node_id); });
};

Cycle& Cycle::operator=(const Cycle& other) {
    if (this != &other) {
        m_nodes_ids.clear();
        other.for_each([this](size_t node_id) { m_nodes_ids.append(node_id); });
    }
    return *this;
}

bool Cycle::empty() const { return m_nodes_ids.empty(); }

size_t Cycle::size() const { return m_nodes_ids.size(); }

void Cycle::insert(size_t index, size_t node_id) { m_nodes_ids.insert(index, node_id); }

void Cycle::add_in_between_if_exists(
    size_t node_id_1, size_t node_id_2, size_t in_between_node_id
) {
    DOMUS_ASSERT(node_id_1 != node_id_2, "Cycle::add_in_between_if_exists: elements are equal");
    DOMUS_ASSERT(
        !has_node(in_between_node_id),
        "Cycle::add_in_between_if_exists: element already exists"
    );
    if (!has_node(node_id_1) || !has_node(node_id_2))
        return;
    size_t node_position_1 = node_position(node_id_1);
    size_t node_position_2 = node_position(node_id_2);
    if (at(node_position_1 + 1) == node_id_2)
        insert(node_position_1, in_between_node_id);
    else if (at(node_position_2 + 1) == node_id_1)
        insert(node_position_2, in_between_node_id);
}

size_t Cycle::operator[](size_t index) const { return m_nodes_ids[index]; }

size_t Cycle::at(size_t index) const { return m_nodes_ids.at(index); }

std::string Cycle::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Cycle: [ ");
    for_each([&](size_t node_id) { std::format_to(out, "{} ", node_id); });
    std::format_to(out, "]\n");
    return result;
}

void Cycle::print() const { std::println("{}", to_string()); }

bool Cycle::has_node(size_t node_id) const { return m_nodes_ids.has_element(node_id); }

size_t Cycle::node_position(size_t node_id) const {
    return m_nodes_ids.element_position(node_id).value();
}

void Cycle::for_each(std::function<void(size_t)> func) const { m_nodes_ids.for_each(func); }