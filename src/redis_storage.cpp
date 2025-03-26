#include "redis_storage.h"

using namespace std::chrono;

std::string* RedisStorage::get(const std::string& key, const time_point<system_clock>& timestamp) {
  auto kv = key_value_store.find(key);
  if (kv == key_value_store.end()) {
    return nullptr;
  }
  StoredValue& stored_value = kv->second;
  if (stored_value.expiry && stored_value.timestamp + *stored_value.expiry < timestamp) {
    return nullptr;
  }
  return &kv->second.value;
}

void RedisStorage::set(std::string& key, std::string& value, time_point<system_clock>& timestamp,
                       std::optional<milliseconds> expiry) {
  StoredValue new_val{.value = std::move(value), .timestamp{std::move(timestamp)}, .expiry{std::move(expiry)}};
  key_value_store.insert_or_assign(std::move(key), std::move(new_val));
}
