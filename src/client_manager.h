#pragma once

#include <loop.h>

#include <memory>
#include <unordered_map>

#include "resp_parser.h"

class CommandProcessor;
class Client;

class ClientManager {
 public:
  ClientManager(EventLoop& event_loop, CommandProcessor& command_processor, int port);
  ClientManager(const ClientManager&) = delete;
  ClientManager& operator=(const ClientManager&) = delete;
  ~ClientManager();

  void accept_client();
  void remove_client(int client_id);
  void submit_command(int client_id, RespCommand& command);
  void send_data_to_client(int client_id, std::vector<char>& data);

 private:
  const int server_fd;
  int client_id_counter = 0;
  std::unordered_map<int, std::unique_ptr<Client>> clients;
  EventLoop& event_loop;
  CommandProcessor& command_processor;

  class AcceptClientTask : public EventloopTask {
   public:
    AcceptClientTask(ClientManager&);
    virtual int fd() const override;
    virtual void trigger() override;

   private:
    ClientManager& client_manager;
  };

  class DeleteClientTask : public EventloopTask {
   public:
    DeleteClientTask(const int event_fd, std::unique_ptr<Client> client_ptr);
    virtual int fd() const override;
    virtual void trigger() override;

   private:
    const int event_fd;
    std::unique_ptr<Client> client_ptr;
  };
};
