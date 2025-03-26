#include "resp_parser.h"

#include <cctype>

#include "client.h"
#include "client_connection.h"
#include "resp_parser.h"

RespParser::RespParser(Client& client, ClientConnection& connection) : client{client}, connection{connection} {
}

void RespParser::parse_forever() {
  parsing_coroutine = start_parsing_coroutine();
}

BasicCoroutine RespParser::start_parsing_coroutine() {
  while (true) {
    uint32_t idx = 0;
    RespCommand command;
    if (!co_await parse_resp_command(idx, command)) {
      break;
    }
    buffer.erase(buffer.begin(), buffer.begin() + idx);
    client.submit_command(command);
  }
  client.disconnect();
  co_return;
}

AwaitableCoroutine<bool> RespParser::parse_resp_command(uint32_t& idx, RespCommand& return_val) {
  if (!co_await ensure_buffer_size(idx, 1)) {
    co_return false;
  }
  if (buffer[idx] != '*') {
    co_return false;
  }
  ++idx;

  int64_t array_size;
  if (!co_await parse_integer(idx, array_size)) {
    co_return false;
  }
  if (array_size > INT_MAX) {
    co_return false;
  }

  for (int i = 0; i < array_size; ++i) {
    std::string resp_string;
    if (!co_await parse_resp_bulk_string(idx, resp_string)) {
      co_return false;
    }
    return_val.emplace_back(std::move(resp_string));
  }
  co_return true;
}

AwaitableCoroutine<bool> RespParser::parse_resp_bulk_string(uint32_t& idx, std::string& return_val) {
  if (!co_await ensure_buffer_size(idx, 1)) {
    co_return false;
  }
  if (buffer[idx] != '$') {
    co_return false;
  }
  ++idx;

  int64_t bulk_string_size;
  if (!co_await parse_integer(idx, bulk_string_size)) {
    co_return false;
  }
  if (bulk_string_size > INT_MAX) {
    co_return false;
  }
  if (!co_await ensure_buffer_size(idx, bulk_string_size)) {
    co_return false;
  }
  return_val = std::string{buffer.begin() + idx, buffer.begin() + idx + bulk_string_size};
  idx += bulk_string_size;
  if (!co_await expect_crlf(idx)) {
    co_return false;
  }
  co_return true;
}

AwaitableCoroutine<bool> RespParser::parse_integer(uint32_t& idx, int64_t& return_val) {
  if (!co_await ensure_buffer_size(idx, 1)) {
    co_return false;
  }

  // Optional sign
  char sign;
  switch (buffer[idx]) {
    case '-':
      sign = -1;
      ++idx;
      break;
    case '+':
      sign = 1;
      ++idx;
      break;
    default:
      sign = 1;
  }

  int64_t integer = 0;
  for (;; ++idx) {
    if (!co_await ensure_buffer_size(idx, 1)) {
      co_return false;
    }
    char c = buffer[idx];
    if (c == '\r') {
      if (!co_await expect_crlf(idx)) {
        co_return false;
      }
      return_val = sign * integer;
      co_return true;
    }
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      co_return false;
    }
    // TODO Check for overflow (depending on sign)
    integer = 10 * integer + (c - '0');
  }
}

AwaitableCoroutine<bool> RespParser::expect_crlf(uint32_t& idx) {
  if (!co_await ensure_buffer_size(idx, 2)) {
    co_return false;
  }
  if (buffer[idx] != '\r') {
    co_return false;
  }
  ++idx;
  if (buffer[idx] != '\n') {
    co_return false;
  }
  ++idx;
  co_return true;
}

AwaitableCoroutine<bool> RespParser::ensure_buffer_size(uint32_t& idx, uint32_t size) {
  while (buffer.size() - idx < size) {
    if (!co_await connection.read_data(buffer)) {
      co_return false;
    }
  }
  co_return true;
}
