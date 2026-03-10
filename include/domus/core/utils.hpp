#pragma once

#include <cassert>
#include <expected>
#include <filesystem>
#include <memory>
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

class MemoryFile {
    struct State {
        char* buffer = nullptr;
        size_t size = 0;
        FILE* mem = nullptr;

        ~State() {
            if (mem)
                fclose(mem);
            if (buffer)
                free(buffer);
        }
    };
    std::unique_ptr<State> state;
    MemoryFile() : state(std::make_unique<State>()) {}

  public:
    MemoryFile(MemoryFile&&) noexcept = default;
    MemoryFile& operator=(MemoryFile&&) noexcept = default;

    MemoryFile(const MemoryFile&) = delete;
    MemoryFile& operator=(const MemoryFile&) = delete;

    static std::expected<MemoryFile, std::string> create() {
        MemoryFile mf;
        mf.state->mem = open_memstream(&mf.state->buffer, &mf.state->size);

        if (!mf.state->mem)
            return std::unexpected("MemoryFile: Failed to open_memstream");
        return mf;
    }

    FILE* get_file() { return state->mem; }

    const char* get_buffer() {
        if (state->mem)
            fflush(state->mem);
        return state->buffer;
    }

    size_t get_size() {
        if (state->mem)
            fflush(state->mem);
        return state->size;
    }
};