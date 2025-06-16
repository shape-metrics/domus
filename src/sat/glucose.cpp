#include "sat/glucose.hpp"

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

#include "core/utils.hpp"

std::string GlucoseResult::to_string() const {
  std::string r = result == GlucoseResultType::SAT ? "SAT" : "UNSAT";
  std::string numbers_str = "Numbers: ";
  for (int num : numbers) numbers_str += std::to_string(num) + " ";
  std::string proof_str = "Proof:\n";
  for (const std::string& line : proof_lines) proof_str += line + "\n";
  return r + "\n" + numbers_str + "\n" + proof_str;
}

void GlucoseResult::print() const { std::cout << to_string() << std::endl; }

GlucoseResult get_results(const std::string& output_file,
                          const std::string& proof_file);

void delete_glucose_temp_files(const std::string& output_file,
                               const std::string& proof_file) {
  remove(output_file.c_str());
  remove(proof_file.c_str());
}

GlucoseResult launch_glucose(const std::string& conjunctive_normal_form_file,
                             bool randomize) {
  const std::string proof_file = get_unique_filename("proof");
  const std::string output_file = get_unique_filename("output");
  std::string command;
  const std::string proof_path = "-certified-output=" + proof_file;
  if (randomize) {
    const std::string rnd_seed =
        "-rnd-seed=" + std::to_string(std::random_device{}());
    command = "./glucose" + conjunctive_normal_form_file + " " + output_file +
              " " + proof_path + " -certified " + "-rnd-init " + rnd_seed;
  } else {
    command = "./glucose " + conjunctive_normal_form_file + " " + output_file +
              " -certified " + proof_path;
  }
  command += " > /dev/null 2>&1";
  std::system(command.c_str());
  GlucoseResult result = get_results(output_file, proof_file);
  delete_glucose_temp_files(output_file, proof_file);
  return std::move(result);
}

std::vector<std::string> get_proof(const std::string& proof_file) {
  std::ifstream file(proof_file.c_str());
  if (!file) throw std::runtime_error("Error: Could not open the file.");
  std::vector<std::string> proof_lines;
  std::string line;
  while (std::getline(file, line)) proof_lines.push_back(line);
  return proof_lines;
}

GlucoseResult get_results(const std::string& output_file,
                          const std::string& proof_file) {
  std::ifstream file(output_file.c_str());
  if (!file) throw std::runtime_error("Error: Could not open the file.");
  std::string line;
  if (std::getline(file, line)) {
    if (line == "UNSAT")
      return GlucoseResult{GlucoseResultType::UNSAT, {}, get_proof(proof_file)};
    std::istringstream iss(line);
    std::vector<int> numbers;
    int num;
    while (iss >> num) numbers.push_back(num);
    return GlucoseResult{GlucoseResultType::SAT, numbers,
                         get_proof(proof_file)};
  }
  throw std::runtime_error("Error: the file is empty.");
}