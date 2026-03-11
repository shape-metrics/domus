#pragma once

#include <expected>
#include <string>
#include <vector>

enum class CnfRowType {
    CLAUSE,
    COMMENT,
};

struct CnfRow {
    CnfRowType m_type;
    std::vector<int> m_clause;
    std::string m_comment;
};

class Cnf {
    size_t m_num_vars = 0;
    size_t m_num_clauses = 0;
    std::vector<CnfRow> m_rows;

  public:
    void add_clause(std::vector<int> clause);
    void add_comment(const std::string& comment);
    size_t get_number_of_variables() const;
    size_t get_number_of_clauses() const;
    std::expected<void, std::string> save_to_file(const std::string& file_path) const;
    std::string to_string() const;
    const std::vector<CnfRow>& get_rows() const;
    void print() const;
};