#ifndef MY_GLUCOSE_H
#define MY_GLUCOSE_H

#include <string>
#include <vector>

enum class GlucoseResultType { SAT, UNSAT };

struct GlucoseResult {
  GlucoseResultType result;
  std::vector<int> numbers;
  std::vector<std::string> proof_lines;
  std::string to_string() const;
  void print() const;
};

GlucoseResult launch_glucose(const std::string& conjunctive_normal_form_file,
                             bool randomize);

#endif