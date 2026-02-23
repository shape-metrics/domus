#include "domus/orthogonal/shape/variables_handler.hpp"
#include "domus/orthogonal/shape/shape.hpp"

#include <cassert>
#include <iostream>

void VariablesHandler::add_variable(int i, int j, const Direction direction) {
    variable_to_edge[m_next_var] = std::make_pair(i, j);
    variable_to_direction[m_next_var] = direction;
    switch (direction) {
    case Direction::UP:
        m_edge_up_variable[{i, j}] = m_next_var;
        m_edge_down_variable[{j, i}] = m_next_var;
        break;
    case Direction::DOWN:
        m_edge_down_variable[{i, j}] = m_next_var;
        m_edge_up_variable[{j, i}] = m_next_var;
        break;
    case Direction::LEFT:
        m_edge_left_variable[{i, j}] = m_next_var;
        m_edge_right_variable[{j, i}] = m_next_var;
        break;
    case Direction::RIGHT:
        m_edge_right_variable[{i, j}] = m_next_var;
        m_edge_left_variable[{j, i}] = m_next_var;
        break;
    case Direction::INVALID:
        assert(false && "Invalid direction");
        break;
    }
    m_next_var++;
}

void VariablesHandler::add_edge_variables(const int i, const int j) {
    add_variable(i, j, Direction::UP);
    add_variable(i, j, Direction::DOWN);
    add_variable(i, j, Direction::LEFT);
    add_variable(i, j, Direction::RIGHT);
}

VariablesHandler::VariablesHandler(const UndirectedGraph& graph) {
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (node_id > neighbor_id)
                continue;
            add_edge_variables(node_id, neighbor_id);
        }
    }
}

int VariablesHandler::get_up_variable(int i, int j) const { return m_edge_up_variable.at({i, j}); }

int VariablesHandler::get_down_variable(int i, int j) const {
    return m_edge_down_variable.at({i, j});
}

int VariablesHandler::get_left_variable(int i, int j) const {
    return m_edge_left_variable.at({i, j});
}

int VariablesHandler::get_right_variable(int i, int j) const {
    return m_edge_right_variable.at({i, j});
}

int VariablesHandler::get_variable(int i, int j, Direction direction) const {
    if (direction == Direction::UP)
        return get_up_variable(i, j);
    if (direction == Direction::DOWN)
        return get_down_variable(i, j);
    if (direction == Direction::LEFT)
        return get_left_variable(i, j);
    if (direction == Direction::RIGHT)
        return get_right_variable(i, j);
    assert(false && "Invalid direction");
    return -1;
}

const std::pair<int, int>& VariablesHandler::get_edge_of_variable(const int variable) const {
    return variable_to_edge.at(variable);
}

Direction VariablesHandler::get_direction_of_edge(int i, int j) const {
    if (get_variable_value(get_up_variable(i, j)))
        return Direction::UP;
    if (get_variable_value(get_down_variable(i, j)))
        return Direction::DOWN;
    if (get_variable_value(get_left_variable(i, j)))
        return Direction::LEFT;
    if (get_variable_value(get_right_variable(i, j)))
        return Direction::RIGHT;
    assert(false && "No direction found for standard edge");
    return Direction::UP;
}

void VariablesHandler::set_variable_value(int variable, bool value) {
    assert(!variable_to_value.contains(variable) && "variable value is already set");
    variable_to_value[variable] = value;
}

bool VariablesHandler::get_variable_value(int variable) const {
    assert(variable_to_value.contains(variable) && "variable does not have a set value");
    return variable_to_value.at(variable);
}

std::string VariablesHandler::to_string() const {
    std::string result = "VariablesHandler:\n";
    for (const auto& [variable, edge] : variable_to_edge) {
        result +=
            ("(" + std::to_string(edge.first) + " -> " + std::to_string(edge.second) +
             "): " + std::to_string(variable) + ", " +
             direction_to_string(variable_to_direction.at(variable)) + "\n");
    }
    return result;
}

void VariablesHandler::print() const { std::cout << to_string() << std::endl; }