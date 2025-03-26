#pragma once

#include <loop.h>

#include <coroutine>
#include <deque>

#include "coroutine.h"

class ClientManager;

class ClientConnection {
 public:
  ClientConnection(int client_fd, ClientManager& client_manager, EventLoop& event_loop);
  ~ClientConnection();

  AwaitableCoroutine<bool> read_data(std::deque<char>& buffer);
  void send_data(std::vector<char>& buffer);

 private:
  const int client_fd;
  ClientManager& client_manager;
  EventLoop& event_loop;
};
