#ifndef MY_CNF_H
#define MY_CNF_H

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
    int m_num_vars = 0;
    int m_num_clauses = 0;
    std::vector<CnfRow> m_rows;

  public:
    void add_clause(std::vector<int> clause);
    void add_comment(const std::string& comment);
    [[nodiscard]] int get_number_of_variables() const;
    [[nodiscard]] int get_number_of_clauses() const;
    void save_to_file(const std::string& file_path) const;
    const std::string to_string() const;
    [[nodiscard]] const std::vector<CnfRow>& get_rows() const;
    void print() const;
};

#endif