#include "loop.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {
constexpr int MAX_EVENTS = 10;
}

EventLoop::EventLoop() : epoll_instance{epoll_create1(0)} {
  if (epoll_instance == -1) {
    throw std::system_error{errno, std::generic_category(), "Failed to create server socket"};
  }
}

EventLoop::~EventLoop() {
  close(epoll_instance);
}

void EventLoop::schedule(std::unique_ptr<EventloopTask> task) {
  schedule(std::move(task), {});
}

void EventLoop::schedule(std::unique_ptr<EventloopTask> task, EventloopTaskFlags flags) {
  const int fd = task->fd();
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
  ev.data.fd = fd;
  int result = epoll_ctl(epoll_instance, EPOLL_CTL_ADD, fd, &ev);
  if (result == -1) {
    throw std::system_error{errno, std::generic_category(), "Error during epoll_ctl"};
  }
  task_fd_map[fd] = {.task = std::move(task), .flags = flags};
}

void EventLoop::remove_task(int fd) {
  task_fd_map.erase(fd);
  int result = epoll_ctl(epoll_instance, EPOLL_CTL_DEL, fd, NULL);
  if (result == -1) {
    throw std::system_error{errno, std::generic_category(), "Error during epoll_ctl"};
  }
}

void EventLoop::run() {
  while (true) {
    std::array<epoll_event, MAX_EVENTS> events;
    int num_events = epoll_wait(epoll_instance, events.begin(), MAX_EVENTS, 100);
    if (num_events == -1) {
      throw std::system_error{errno, std::generic_category(), "Error during epoll_wait"};
    }
    for (int i = 0; i < num_events; ++i) {
      int fd = events[i].data.fd;
      StoredEventloopTask& stored_task{task_fd_map[fd]};
      if (stored_task.flags.schedule_forever) {
        stored_task.task->trigger();
        continue;
      }
      std::unique_ptr<EventloopTask> task{std::move(stored_task.task)};
      remove_task(fd);
      task->trigger();
    }
  }
}
