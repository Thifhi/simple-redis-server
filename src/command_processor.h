#pragma once

#include <loop.h>

#include <chrono>
#include <queue>

#include "resp_parser.h"

class ClientManager;
class RedisStorage;

class CommandProcessor {
 public:
  CommandProcessor(EventLoop& event_loop, RedisStorage& redis_storage);
  // TODO delete copy constructors?
  ~CommandProcessor();
  void set_client_manager(ClientManager* client_manager);
  void submit_command(int client_id, RespCommand& command);

 private:
  struct ClientCommand {
    int client_id;
    RespCommand cmd_array;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
  };

  class CommandEventsTask : public EventloopTask {
   public:
    CommandEventsTask(CommandProcessor& command_processor);
    virtual int fd() const override;
    virtual void trigger() override;

   private:
    CommandProcessor& command_processor;
  };

  EventLoop& event_loop;
  RedisStorage& redis_storage;
  ClientManager* client_manager = nullptr;
  int queue_event_fd;
  std::queue<ClientCommand> command_queue;

  void process_queue();
  void process_command(ClientCommand& command);
  void process_ping(ClientCommand& command);
  void process_echo(ClientCommand& command);
  void process_get(ClientCommand& command);
  void process_set(ClientCommand& command);
  void process_info(ClientCommand& command);
  void command_error(ClientCommand& command);

  using CommandHandler = void (CommandProcessor::*)(ClientCommand&);
  inline static const std::unordered_map<std::string, CommandHandler> COMMAND_MAP = {
      {"COMMAND", &CommandProcessor::process_ping},  //
      {"PING", &CommandProcessor::process_ping},     //
      {"ECHO", &CommandProcessor::process_echo},     //
      {"GET", &CommandProcessor::process_get},       //
      {"SET", &CommandProcessor::process_set},       //
      {"INFO", &CommandProcessor::process_info},     //
  };
};
