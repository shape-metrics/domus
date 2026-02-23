#ifndef MY_CONFIG_H
#define MY_CONFIG_H

#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class Config {
  public:
    virtual ~Config() = default;
    virtual std::optional<std::string> get(std::string_view key) const = 0;
    std::string get_or(std::string_view key, std::string_view default_value) const {
        return get(key).value_or(std::string(default_value));
    }
    static std::expected<std::unique_ptr<Config>, std::string> create(std::filesystem::path path);

  protected:
    Config() = default;
};

#endif