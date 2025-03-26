#pragma once

#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

struct EventloopTaskFlags {
  bool schedule_forever = false;
};

class EventloopTask {
 public:
  virtual int fd() const = 0;
  virtual void trigger() = 0;
  virtual ~EventloopTask() = default;
};

class EventLoop {
 public:
  EventLoop();
  EventLoop(const EventLoop&) = delete;
  EventLoop& operator=(const EventLoop&) = delete;
  ~EventLoop();
  void schedule(std::unique_ptr<EventloopTask> task);
  void schedule(std::unique_ptr<EventloopTask> task, EventloopTaskFlags flags);
  void remove_task(int fd);
  void run();

 private:
  struct StoredEventloopTask {
    std::unique_ptr<EventloopTask> task;
    EventloopTaskFlags flags;
  };

  const int epoll_instance;
  std::unordered_map<int, StoredEventloopTask> task_fd_map;
};