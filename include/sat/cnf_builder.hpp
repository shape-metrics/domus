#ifndef MY_CNF_BUILDER_H
#define MY_CNF_BUILDER_H

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

class CnfBuilder {
 private:
  int m_num_vars = 0;
  int m_num_clauses = 0;
  std::vector<CnfRow> m_rows;

 public:
  void add_clause(std::vector<int> clause);
  void add_comment(const std::string& comment);
  int get_number_of_variables() const;
  int get_number_of_clauses() const;
  void convert_to_cnf(const std::string& file_path) const;
};

#endif