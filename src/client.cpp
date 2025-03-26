#include "client.h"

#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <coroutine>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include "client_manager.h"

namespace {
constexpr int RECV_CHUNK_SIZE = 2048;
}

Client::Client(const int client_id, const int client_fd, ClientManager& client_manager, EventLoop& event_loop)
    : client_id{client_id},
      client_manager{client_manager},
      connection{client_fd, client_manager, event_loop},
      parser{*this, connection} {
  parser.parse_forever();
}

Client::~Client() {
  SPDLOG_INFO("Disconnected client {}", this->client_id);
}

void Client::submit_command(RespCommand& command) {
  client_manager.submit_command(client_id, command);
}

void Client::send_data(std::vector<char>& data) {
  connection.send_data(data);
}

void Client::disconnect() {
  client_manager.remove_client(client_id);
}
