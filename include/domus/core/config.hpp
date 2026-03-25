#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace domus {

class Config {
  public:
    virtual ~Config() = default;
    virtual std::optional<std::string> get(const std::string& key) const = 0;
    std::string get_or(const std::string& key, std::string_view default_value) const {
        return get(key).value_or(std::string(default_value));
    }
    static std::expected<std::unique_ptr<Config>, std::string> create(std::filesystem::path path);

  protected:
    Config() = default;
};

} // namespace domus