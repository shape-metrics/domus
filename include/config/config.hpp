#ifndef CONFIG_H
#define CONFIG_H

#include <memory>
#include <string>

class ConfigImpl;

class Config {
 private:
  std::unique_ptr<ConfigImpl> m_config_impl;

 public:
  Config(const std::string& filename);
  const std::string& get(const std::string& key) const;
  ~Config();
};

#endif
