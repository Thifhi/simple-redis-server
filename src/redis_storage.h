#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

struct StoredValue {
  std::string value;
  std::chrono::time_point<std::chrono::system_clock> timestamp;
  std::optional<std::chrono::milliseconds> expiry;
};

class RedisStorage {
 public:
  std::string* get(const std::string& key, const std::chrono::time_point<std::chrono::system_clock>& timestamp);
  void set(std::string& key, std::string& value, std::chrono::time_point<std::chrono::system_clock>& timestamp,
           std::optional<std::chrono::milliseconds> expiry);

 private:
  std::unordered_map<std::string, StoredValue> key_value_store;
};
