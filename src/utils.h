#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

inline bool ichar_equals(char a, char b) {
  return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}

inline bool iequals(std::string_view lhs, std::string_view rhs) {
  return std::ranges::equal(lhs, rhs, ichar_equals);
}

inline void str_toupper_inplace(std::string& s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
}
