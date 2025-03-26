#include <arpa/inet.h>
#include <loop.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include "client_manager.h"
#include "command_processor.h"
#include "redis_storage.h"

struct SocketAddress {
  std::string host;
  int port;
};

std::istream &operator>>(std::istream &is, std::optional<SocketAddress> &return_val) {
  SocketAddress socket_address;
  is >> socket_address.host;
  is >> socket_address.port;
  return_val = socket_address;
  return is;
}

#include <tclap/CmdLine.h>

using namespace TCLAP;

int main(int argc, char **argv) {
  int port;
  std::optional<SocketAddress> replication_master_host_port;
  try {
    CmdLine cmd("Redis Server", ' ', "420");
    ValueArg<int> port_arg("p", "port", "Port to host the server", false, 6379, "integer");
    cmd.add(port_arg);

    ValueArg<std::optional<SocketAddress>> replicaof_arg("", "replicaof", "Master to replicate", false, std::nullopt,
                                                         "<MASTER_HOST> <MASTER_PORT>");
    cmd.add(replicaof_arg);

    cmd.parse(argc, argv);
    port = port_arg.getValue();
    replication_master_host_port = replicaof_arg.getValue();
  } catch (ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return 1;
  }

  EventLoop event_loop;
  RedisStorage redis_storage;
  CommandProcessor command_processor{event_loop, redis_storage};
  ClientManager client_manager{event_loop, command_processor, port};
  event_loop.run();
  return 0;
}
