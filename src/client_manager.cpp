#include "client_manager.h"

#include <arpa/inet.h>
#include <spdlog/spdlog.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include "client.h"
#include "command_processor.h"
#include "resp_parser.h"

ClientManager::ClientManager(EventLoop &event_loop, CommandProcessor &command_processor, int port)
    : server_fd{socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)},
      event_loop{event_loop},
      command_processor{command_processor} {
  if (server_fd == -1) {
    throw std::system_error{errno, std::generic_category(), "Failed to create server socket"};
  }
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    throw std::system_error{errno, std::generic_category(), "Failed to set socket option for server socket"};
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
    throw std::system_error{errno, std::generic_category(), "Server socket failed to bind"};
  }

  if (listen(server_fd, 4096) != 0) {
    throw std::system_error{errno, std::generic_category(), "Server socket failed to listen"};
  }
  event_loop.schedule(std::make_unique<AcceptClientTask>(*this), {.schedule_forever = true});
  command_processor.set_client_manager(this);
}

ClientManager::~ClientManager() {
  event_loop.remove_task(server_fd);
  close(server_fd);
}

void ClientManager::accept_client() {
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  // TODO accept all clients in a loop, not just one
  // TODO ai slop nonblock flag, check with the book
  // TODO address can be null since not used anyway? refer to man or book
  int client_fd = accept4(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len, SOCK_NONBLOCK);
  if (client_fd == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    }
    throw std::system_error{errno, std::generic_category(), "Server socket failed to accept"};
  }
  clients[client_id_counter] = std::make_unique<Client>(client_id_counter, client_fd, *this, event_loop);
  SPDLOG_INFO("Connected client {}", client_id_counter);
  ++client_id_counter;
}

void ClientManager::remove_client(const int client_id) {
  // We can not directly destruct the client because this function is called from the coroutine that the client itself
  // owns. Instead we move the client to a task and make the task immediately trigger. Upon the execution of this task
  // (which is a no-op), it will be destructed along with the contained client.
  const int del_event_fd = eventfd(0, 0);
  event_loop.schedule(std::make_unique<DeleteClientTask>(del_event_fd, std::move(clients[client_id])));
  eventfd_write(del_event_fd, 1);
  clients.erase(client_id);
}

void ClientManager::submit_command(const int client_id, RespCommand &command) {
  command_processor.submit_command(client_id, command);
}

void ClientManager::send_data_to_client(const int client_id, std::vector<char> &data) {
  auto it = clients.find(client_id);
  if (it == clients.end()) {
    return;
  }
  std::unique_ptr<Client> &client = it->second;
  client->send_data(data);
}

ClientManager::AcceptClientTask::AcceptClientTask(ClientManager &client_manager) : client_manager{client_manager} {
}

int ClientManager::AcceptClientTask::fd() const {
  return client_manager.server_fd;
}

void ClientManager::AcceptClientTask::trigger() {
  client_manager.accept_client();
}

ClientManager::DeleteClientTask::DeleteClientTask(const int event_fd, std::unique_ptr<Client> client_ptr)
    : event_fd{event_fd}, client_ptr{std::move(client_ptr)} {
}

int ClientManager::DeleteClientTask::fd() const {
  return event_fd;
}

void ClientManager::DeleteClientTask::trigger() {
}
