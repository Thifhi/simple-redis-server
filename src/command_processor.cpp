#include "command_processor.h"

#include <loop.h>
#include <sys/eventfd.h>

#include <chrono>
#include <ranges>
#include <system_error>

#include "client_manager.h"
#include "redis_storage.h"
#include "resp_encoders.h"
#include "utils.h"

using namespace std::chrono;

CommandProcessor::CommandProcessor(EventLoop& event_loop, RedisStorage& redis_storage)
    : event_loop{event_loop}, redis_storage{redis_storage}, queue_event_fd(eventfd(0, 0)) {
  if (queue_event_fd == -1) {
    throw std::system_error{errno, std::generic_category(), "Create queue event_fd failed"};
  }
  event_loop.schedule(std::make_unique<CommandEventsTask>(*this), {.schedule_forever = true});
}

CommandProcessor::~CommandProcessor() {
  event_loop.remove_task(queue_event_fd);
  close(queue_event_fd);
}

void CommandProcessor::set_client_manager(ClientManager* client_manager) {
  this->client_manager = client_manager;
}

void CommandProcessor::submit_command(int client_id, RespCommand& command) {
  ClientCommand new_command{.client_id = client_id, .cmd_array = std::move(command), .timestamp = system_clock::now()};
  command_queue.emplace(std::move(new_command));
  eventfd_write(queue_event_fd, 1);
}

void CommandProcessor::process_queue() {
  uint64_t queue_size;
  eventfd_read(queue_event_fd, &queue_size);

  // TODO remove later
  if (queue_size != command_queue.size()) {
    throw std::system_error{errno, std::generic_category(), "queue size not equal?"};
  }

  int process_elems = command_queue.size();
  for (int i = 0; i < process_elems; ++i) {
    ClientCommand& client_command = command_queue.front();
    process_command(client_command);
    command_queue.pop();
  }
}

void CommandProcessor::process_command(ClientCommand& command) {
  if (command.cmd_array.size() < 1) {
    command_error(command);
    return;
  }
  str_toupper_inplace(command.cmd_array[0]);
  auto cmd_handler_it = COMMAND_MAP.find(command.cmd_array[0]);
  if (cmd_handler_it == COMMAND_MAP.end()) {
    command_error(command);
    return;
  }
  (this->*(cmd_handler_it->second))(command);
}

void CommandProcessor::process_ping(ClientCommand& command) {
  std::vector<char> data = encode_simple_string("PONG");
  client_manager->send_data_to_client(command.client_id, data);
}

void CommandProcessor::process_echo(ClientCommand& command) {
  if (command.cmd_array.size() != 2) {
    command_error(command);
    return;
  }
  std::vector<char> data = encode_bulk_string(command.cmd_array[1]);
  client_manager->send_data_to_client(command.client_id, data);
}

void CommandProcessor::process_get(ClientCommand& command) {
  if (command.cmd_array.size() != 2) {
    command_error(command);
    return;
  }
  std::string* val = redis_storage.get(command.cmd_array[1], command.timestamp);
  std::vector<char> data;
  if (!val) {
    data = encode_nil_bulk_string();
  } else {
    data = encode_bulk_string(*val);
  }
  client_manager->send_data_to_client(command.client_id, data);
}

void CommandProcessor::process_set(ClientCommand& command) {
  if (command.cmd_array.size() < 3) {
    command_error(command);
    return;
  }
  std::optional<milliseconds> expiry;

  // TODO hack. actually parse later DX
  if (command.cmd_array.size() == 5) {
    try {
      expiry = milliseconds{std::stoi(command.cmd_array[4])};
    } catch (...) {  // TODO be more specific with error
      command_error(command);
      return;
    }
  }

  redis_storage.set(command.cmd_array[1], command.cmd_array[2], command.timestamp, expiry);
  std::vector<char> data = encode_simple_string("OK");
  client_manager->send_data_to_client(command.client_id, data);
}

void CommandProcessor::process_info(ClientCommand& command) {
  if (command.cmd_array.size() != 2) {  // TODO actually parse at some point
    command_error(command);
    return;
  }
  std::vector<char> data = encode_bulk_string("role:master");
  client_manager->send_data_to_client(command.client_id, data);
}

void CommandProcessor::command_error(ClientCommand& command) {
  // TODO use command in error at some point
  (void)command;
  std::vector<char> data = encode_simple_error("ERR command");
  client_manager->send_data_to_client(command.client_id, data);
}

CommandProcessor::CommandEventsTask::CommandEventsTask(CommandProcessor& command_processor)
    : command_processor{command_processor} {
}

int CommandProcessor::CommandEventsTask::fd() const {
  return command_processor.queue_event_fd;
}

void CommandProcessor::CommandEventsTask::trigger() {
  command_processor.process_queue();
}
