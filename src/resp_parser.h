#pragma once

#include <deque>
#include <string>
#include <vector>

#include "coroutine.h"

class Client;
class ClientConnection;

using RespCommand = std::vector<std::string>;

class RespParser {
 public:
  RespParser(Client& client, ClientConnection& connection);
  void parse_forever();

 private:
  Client& client;
  ClientConnection& connection;
  BasicCoroutine parsing_coroutine;
  std::deque<char> buffer;

  BasicCoroutine start_parsing_coroutine();
  AwaitableCoroutine<bool> parse_resp_command(uint32_t& idx, RespCommand& return_val);
  AwaitableCoroutine<bool> parse_resp_bulk_string(uint32_t& idx, std::string& return_val);
  AwaitableCoroutine<bool> parse_integer(uint32_t& idx, int64_t& return_val);
  AwaitableCoroutine<bool> expect_crlf(uint32_t& idx);
  AwaitableCoroutine<bool> ensure_buffer_size(uint32_t& idx, uint32_t size);
};
