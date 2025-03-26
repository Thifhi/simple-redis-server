#pragma once

#include <string>
#include <vector>

namespace detail {
constexpr std::string_view CRLF = "\r\n";
constexpr std::string_view NIL_BULK_STRING = "$-1\r\n";

inline void insert_crlf(std::vector<char>& result) {
  result.insert(result.end(), CRLF.begin(), CRLF.end());
}

template <char INITIAL_CHAR>
inline std::vector<char> encode_simple(const std::string_view& data) {
  std::vector<char> result;
  result.reserve(1 + data.size() + detail::CRLF.size());
  result.emplace_back(INITIAL_CHAR);
  result.insert(result.end(), data.begin(), data.end());
  detail::insert_crlf(result);
  return result;
}

template <char INITIAL_CHAR>
inline void encode_bulk(const std::string_view& data, std::vector<char>& result) {
  std::string data_size_str = std::to_string(data.size());
  result.reserve(result.size() + 1 + data_size_str.size() + detail::CRLF.size() + data.size() + detail::CRLF.size());
  result.emplace_back(INITIAL_CHAR);
  result.insert(result.end(), data_size_str.begin(), data_size_str.end());
  detail::insert_crlf(result);
  result.insert(result.end(), data.begin(), data.end());
  detail::insert_crlf(result);
}

inline void encode_bulk_string(const std::string_view& data, std::vector<char>& result) {
  encode_bulk<'$'>(data, result);
}

inline void encode_bulk_error(const std::string_view& data, std::vector<char>& result) {
  encode_bulk<'!'>(data, result);
}
}  // namespace detail

inline std::vector<char> encode_bulk_string(const std::string_view& data) {
  std::vector<char> result;
  detail::encode_bulk_string(data, result);
  return result;
}

inline std::vector<char> encode_bulk_error(const std::string_view& data) {
  std::vector<char> result;
  detail::encode_bulk_error(data, result);
  return result;
}

inline std::vector<char> encode_simple_string(const std::string_view& data) {
  return detail::encode_simple<'+'>(data);
}

inline std::vector<char> encode_simple_error(const std::string_view& data) {
  return detail::encode_simple<'-'>(data);
}

inline std::vector<char> encode_array_of_bulk_string(const std::vector<std::string_view>& data) {
  std::vector<char> result;
  std::string data_size_str = std::to_string(data.size());
  result.reserve(1 + data_size_str.size() + detail::CRLF.size());
  result.emplace_back('*');
  result.insert(result.end(), data_size_str.begin(), data_size_str.end());
  detail::insert_crlf(result);
  for (const std::string_view& str_view : data) {
    detail::encode_bulk_string(str_view, result);
  }
  return result;
}

inline std::vector<char> encode_nil_bulk_string() {
  std::vector<char> result;
  result.reserve(detail::NIL_BULK_STRING.size());
  result.insert(result.end(), detail::NIL_BULK_STRING.begin(), detail::NIL_BULK_STRING.end());
  return result;
}
