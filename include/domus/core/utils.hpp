#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

std::expected<void, std::string>
save_string_to_file(const std::string& filename, const std::string& content);

std::expected<std::vector<std::string>, std::string>
collect_txt_files(std::filesystem::path folder_path);

double compute_stddev(const std::vector<size_t>& values);