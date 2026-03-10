#pragma once

#include <expected>
#include <memory>
#include <string>

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
