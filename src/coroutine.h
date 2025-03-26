#pragma once

#include <loop.h>

#include <coroutine>
#include <memory>
#include <type_traits>
#include <variant>

template <typename Result = void>
class [[nodiscard]] AwaitableCoroutine {
 public:
  AwaitableCoroutine() = default;

  struct AwaitableCoroutinePromise {
    Result stored_result;
    std::coroutine_handle<> continuation;

    struct FinalAwaitable {
      bool await_ready() noexcept {
        return false;
      }

      auto await_suspend(std::coroutine_handle<AwaitableCoroutinePromise> handle) noexcept {
        return handle.promise().continuation;
      }

      void await_resume() noexcept {
      }
    };

    AwaitableCoroutine get_return_object() {
      return {std::coroutine_handle<AwaitableCoroutinePromise>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() {
      return {};
    }

    FinalAwaitable final_suspend() noexcept {
      return {};
    }

    void return_value(Result&& passed_result) {
      stored_result = std::move(passed_result);
    }

    void unhandled_exception() {
    }
  };

  bool await_ready() {
    return false;
  }

  auto await_suspend(std::coroutine_handle<> calling) {
    handle.promise().continuation = calling;
    return handle;
  }

  template <typename T = Result>
    requires(!std::is_same_v<T, void>)
  Result await_resume() {
    return handle.promise().stored_result;
  }

  template <typename T = Result>
    requires(std::is_same_v<T, void>)
  void await_resume() {
    return;
  }

  using promise_type = AwaitableCoroutinePromise;

  ~AwaitableCoroutine() {
    handle.destroy();
  }

 private:
  std::coroutine_handle<AwaitableCoroutinePromise> handle;
  AwaitableCoroutine(std::coroutine_handle<AwaitableCoroutinePromise> handle) : handle(handle) {
  }
};

template <>
struct AwaitableCoroutine<void>::AwaitableCoroutinePromise {
  std::coroutine_handle<> continuation;
  struct FinalAwaitable {
    bool await_ready() noexcept {
      return false;
    }

    auto await_suspend(std::coroutine_handle<AwaitableCoroutinePromise> handle) noexcept {
      return handle.promise().continuation;
    }

    void await_resume() noexcept {
    }
  };

  AwaitableCoroutine get_return_object() {
    return {std::coroutine_handle<AwaitableCoroutinePromise>::from_promise(*this)};
  }

  std::suspend_always initial_suspend() {
    return {};
  }

  FinalAwaitable final_suspend() noexcept {
    return {};
  }

  void return_void() {
  }

  void unhandled_exception() {
  }
};

class BasicCoroutine {
 public:
  struct BasicCoroutinePromise {
    BasicCoroutine get_return_object() {
      return {std::coroutine_handle<BasicCoroutinePromise>::from_promise(*this)};
    }

    std::suspend_never initial_suspend() {
      return {};
    }

    std::suspend_always final_suspend() noexcept {
      return {};
    }

    void return_void() {
    }

    void unhandled_exception() {
      // TODO
      // these coroutines will probably be used as fire-and-forget
      // maybe we should log the error here?
    }
  };

  using promise_type = BasicCoroutinePromise;

  BasicCoroutine() = default;

  BasicCoroutine(const BasicCoroutine&) = delete;
  BasicCoroutine& operator=(const BasicCoroutine&) = delete;

  BasicCoroutine(BasicCoroutine&& other) {
    handle = other.handle;
    other.handle = nullptr;
  }

  BasicCoroutine& operator=(BasicCoroutine&& other) {
    handle = other.handle;
    other.handle = nullptr;
    return *this;
  }

  ~BasicCoroutine() {
    if (handle) {
      handle.destroy();
    }
  }

 private:
  std::coroutine_handle<BasicCoroutinePromise> handle;
  BasicCoroutine(std::coroutine_handle<BasicCoroutinePromise> handle) : handle(handle) {
  }
};

class EventloopAwaitableTask : public EventloopTask {
 public:
  EventloopAwaitableTask(int event_loop_fd, std::coroutine_handle<> handle)
      : event_loop_fd{event_loop_fd}, handle{handle} {
  }

  virtual int fd() const override {
    return event_loop_fd;
  }
  virtual void trigger() override {
    handle.resume();
  }

 private:
  const int event_loop_fd;
  std::coroutine_handle<> handle;
};

class EventloopAwaitable {
 public:
  EventloopAwaitable(EventLoop& event_loop, int event_loop_fd) : event_loop{event_loop}, event_loop_fd{event_loop_fd} {
  }

  bool await_ready() noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> calling) noexcept {
    event_loop.schedule(std::make_unique<EventloopAwaitableTask>(event_loop_fd, calling));
  }

  void await_resume() noexcept {
  }

 private:
  EventLoop& event_loop;
  const int event_loop_fd;
};
