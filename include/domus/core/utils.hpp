#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace domus::utilities {

std::expected<void, std::string>
save_string_to_file(std::filesystem::path path, const std::string_view content);

std::expected<std::vector<std::string>, std::string>
collect_txt_files(std::filesystem::path folder_path);

double compute_stddev(const std::vector<size_t>& values);

} // namespace domus::utilities