#include "domus/orthogonal/shape/variables_handler.hpp"

#include <cassert>
#include <print>

#include "domus/core/graph/graph.hpp"

void VariablesHandler::add_variable(size_t i, size_t j, const Direction direction) {
    variable_to_edge.push_back({i, j});
    variable_to_direction.push_back(direction);
    variable_to_value.push_back(-1);
    switch (direction) {
    case Direction::UP:
        m_edge_up_variable.add(i, j, m_next_var);
        m_edge_down_variable.add(j, i, m_next_var);
        break;
    case Direction::DOWN:
        m_edge_down_variable.add(i, j, m_next_var);
        m_edge_up_variable.add(j, i, m_next_var);
        break;
    case Direction::LEFT:
        m_edge_left_variable.add(i, j, m_next_var);
        m_edge_right_variable.add(j, i, m_next_var);
        break;
    case Direction::RIGHT:
        m_edge_right_variable.add(i, j, m_next_var);
        m_edge_left_variable.add(j, i, m_next_var);
        break;
    case Direction::INVALID:
        assert(false && "VariablesHandler::add_variable: invalid direction");
        break;
    }
    m_next_var++;
}

void VariablesHandler::add_edge_variables(size_t i, size_t j) {
    add_variable(i, j, Direction::UP);
    add_variable(i, j, Direction::DOWN);
    add_variable(i, j, Direction::LEFT);
    add_variable(i, j, Direction::RIGHT);
}

VariablesHandler::VariablesHandler(const Graph& graph) {
    variable_to_edge.push_back({0, 0});
    variable_to_direction.push_back(Direction::INVALID);
    variable_to_value.push_back(-1);
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (node_id > neighbor_id)
                return;
            add_edge_variables(node_id, neighbor_id);
        });
    });
}

size_t VariablesHandler::get_up_variable(size_t i, size_t j) const {
    return m_edge_up_variable.get(i, j);
}

size_t VariablesHandler::get_down_variable(size_t i, size_t j) const {
    return m_edge_down_variable.get(i, j);
}

size_t VariablesHandler::get_left_variable(size_t i, size_t j) const {
    return m_edge_left_variable.get(i, j);
}

size_t VariablesHandler::get_right_variable(size_t i, size_t j) const {
    return m_edge_right_variable.get(i, j);
}

size_t VariablesHandler::get_variable(size_t i, size_t j, Direction direction) const {
    if (direction == Direction::UP)
        return get_up_variable(i, j);
    if (direction == Direction::DOWN)
        return get_down_variable(i, j);
    if (direction == Direction::LEFT)
        return get_left_variable(i, j);
    if (direction == Direction::RIGHT)
        return get_right_variable(i, j);
    assert(false && "Invalid direction");
    return 0;
}

const Edge& VariablesHandler::get_edge_of_variable(size_t variable) const {
    return variable_to_edge.at(variable);
}

Direction VariablesHandler::get_direction_of_edge(size_t i, size_t j) const {
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

void VariablesHandler::set_variable_value(size_t variable, bool value) {
    assert(
        variable_to_value.at(variable) == -1 &&
        "VariablesHandler::set_variable_value: variable value is already set"
    );
    variable_to_value[variable] = value;
}

bool VariablesHandler::get_variable_value(size_t variable) const {
    assert(
        variable_to_value.at(variable) != -1 &&
        "VariablesHandler::get_variable_value: variable does not have a set value"
    );
    return variable_to_value.at(variable);
}

std::string VariablesHandler::to_string() const {
    std::string result = "VariablesHandler:\n";
    for (size_t variable = 1; variable < variable_to_edge.size(); variable++) {
        const Edge& edge = variable_to_edge.at(variable);
        result +=
            ("(" + std::to_string(edge.from_id) + " -> " + std::to_string(edge.to_id) +
             "): " + std::to_string(variable) + ", " +
             direction_to_string(variable_to_direction.at(variable)) + "\n");
    }
    return result;
}

void VariablesHandler::print() const { println("{}", to_string()); }