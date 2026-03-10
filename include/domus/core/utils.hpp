#pragma once

#include <cassert>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

std::expected<void, std::string>
save_string_to_file(const std::string& filename, const std::string& content);

enum class Color {
    RED,
    RED_SPECIAL,
    BLUE,
    BLUE_DARK,
    BLACK,
    GREEN,
    GREEN_DARK,
    RED_AND_BLUE,
    NONE,
    ANY,
};

std::string color_to_string(Color color);

Color string_to_color(const std::string& color);

std::expected<std::vector<std::string>, std::string>
collect_txt_files(std::filesystem::path folder_path);

double compute_stddev(const std::vector<int>& values);