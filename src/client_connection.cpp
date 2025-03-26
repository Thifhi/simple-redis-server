#include "client_connection.h"

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <stdexcept>
#include <system_error>

#include "client_manager.h"

namespace {
constexpr int RECV_CHUNK_SIZE = 1024;
}

ClientConnection::ClientConnection(const int client_fd, ClientManager& client_manager, EventLoop& event_loop)
    : client_fd{client_fd}, client_manager{client_manager}, event_loop{event_loop} {
}

AwaitableCoroutine<bool> ClientConnection::read_data(std::deque<char>& buffer) {
  co_await EventloopAwaitable(event_loop, client_fd);
  while (true) {
    std::array<char, RECV_CHUNK_SIZE> received{};
    ssize_t n_bytes = recv(client_fd, &received, sizeof(received), 0);
    if (n_bytes == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        co_return true;
      }
      // Log: throw std::system_error{errno, std::generic_category(), "Client socket failed to recv"};
      co_return false;
    }
    if (n_bytes == 0) {
      co_return false;
    }
    buffer.insert(buffer.end(), received.begin(), received.begin() + n_bytes);
  }
}

ClientConnection::~ClientConnection() {
  close(client_fd);
}

void ClientConnection::send_data(std::vector<char>& buffer) {
  // TODO non-blocking semantics? Do a send queue?
  send(client_fd, buffer.data(), buffer.size(), 0);
}
