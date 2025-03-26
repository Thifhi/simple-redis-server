#pragma once

#include <loop.h>

#include "client_connection.h"
#include "coroutine.h"
#include "resp_parser.h"

class ClientManager;

class Client {
 public:
  Client(int client_id, int client_fd, ClientManager& client_manager, EventLoop& event_loop);
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  ~Client();

  void submit_command(RespCommand& command);
  void send_data(std::vector<char>& data);
  void disconnect();

 private:
  const int client_id;
  ClientManager& client_manager;
  ClientConnection connection;
  RespParser parser;
};
