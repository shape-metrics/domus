#include "variables_handler.hpp"

#include <format>
#include <print>

#include "domus/core/graph/graph.hpp"

#include "../../core/domus_debug.hpp"

namespace domus::orthogonal::shape {

void VariablesHandler::add_variable(size_t edge_id, const Direction direction) {
    m_variable_to_edge_id.push_back(edge_id);
    m_variable_to_direction.push_back(direction);
    m_variable_to_value.push_back(-1);
    switch (direction) {
    case Direction::UP:
        m_edge_up_variable[edge_id] = m_next_var;
        break;
    case Direction::DOWN:
        m_edge_down_variable[edge_id] = m_next_var;
        break;
    case Direction::LEFT:
        m_edge_left_variable[edge_id] = m_next_var;
        break;
    case Direction::RIGHT:
        m_edge_right_variable[edge_id] = m_next_var;
        break;
    case Direction::INVALID:
        DOMUS_ASSERT(false, "VariablesHandler::add_variable: invalid direction");
        break;
    }
    m_next_var++;
}

void VariablesHandler::add_edge_variables(size_t edge_id) {
    add_variable(edge_id, Direction::UP);
    add_variable(edge_id, Direction::DOWN);
    add_variable(edge_id, Direction::LEFT);
    add_variable(edge_id, Direction::RIGHT);
}

VariablesHandler::VariablesHandler(const graph::Graph& graph) {
    m_variable_to_edge_id.push_back(graph.get_number_of_edges());
    m_variable_to_direction.push_back(Direction::INVALID);
    m_variable_to_value.push_back(-1);
    m_edge_up_variable.resize(graph.get_number_of_edges());
    m_edge_down_variable.resize(graph.get_number_of_edges());
    m_edge_left_variable.resize(graph.get_number_of_edges());
    m_edge_right_variable.resize(graph.get_number_of_edges());
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_out_edge(node_id, [&](graph::EdgeIter edge) {
            add_edge_variables(edge.id);
        });
    });
}

size_t VariablesHandler::get_up_variable(size_t edge_id) const {
    return m_edge_up_variable.at(edge_id).value();
}

size_t VariablesHandler::get_down_variable(size_t edge_id) const {
    return m_edge_down_variable.at(edge_id).value();
}

size_t VariablesHandler::get_left_variable(size_t edge_id) const {
    return m_edge_left_variable.at(edge_id).value();
}

size_t VariablesHandler::get_right_variable(size_t edge_id) const {
    return m_edge_right_variable.at(edge_id).value();
}

size_t VariablesHandler::get_variable(size_t edge_id, Direction direction) const {
    if (direction == Direction::UP)
        return get_up_variable(edge_id);
    if (direction == Direction::DOWN)
        return get_down_variable(edge_id);
    if (direction == Direction::LEFT)
        return get_left_variable(edge_id);
    if (direction == Direction::RIGHT)
        return get_right_variable(edge_id);
    DOMUS_ASSERT(false, "VariablesHandler::get_variable: invalid direction");
    return 0;
}

size_t VariablesHandler::get_edge_id_of_variable(size_t variable) const {
    return m_variable_to_edge_id.at(variable);
}

Direction VariablesHandler::get_direction_of_edge(size_t edge_id) const {
    if (get_variable_value(get_up_variable(edge_id)))
        return Direction::UP;
    if (get_variable_value(get_down_variable(edge_id)))
        return Direction::DOWN;
    if (get_variable_value(get_left_variable(edge_id)))
        return Direction::LEFT;
    if (get_variable_value(get_right_variable(edge_id)))
        return Direction::RIGHT;
    DOMUS_ASSERT(
        false,
        "VariablesHandler::get_direction_of_edge: no direction found for standard edge"
    );
    return Direction::UP;
}

void VariablesHandler::set_variable_value(size_t variable, bool value) {
    DOMUS_ASSERT(
        m_variable_to_value.at(variable) == -1,
        "VariablesHandler::set_variable_value: variable value is already set"
    );
    m_variable_to_value[variable] = value;
}

bool VariablesHandler::get_variable_value(size_t variable) const {
    DOMUS_ASSERT(
        m_variable_to_value.at(variable) != -1,
        "VariablesHandler::get_variable_value: variable does not have a set value"
    );
    return m_variable_to_value.at(variable);
}

std::string VariablesHandler::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "VariablesHandler:\n");
    for (size_t variable = 1; variable < m_variable_to_edge_id.size(); variable++) {
        const size_t edge_id = m_variable_to_edge_id.at(variable);
        std::format_to(
            out,
            "(edge_id: {}): {}, {}\n",
            edge_id,
            variable,
            direction_to_string(m_variable_to_direction.at(variable))
        );
    }
    return result;
}

void VariablesHandler::print() const { println("{}", to_string()); }

} // namespace domus::orthogonal::shape