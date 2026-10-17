#pragma once

#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <memory>
#include <mutex>
#include <utility>

namespace vix::network::test_detail
{
  struct FinalSuspendState
  {
    std::mutex mutex;
    std::condition_variable cv;
    bool suspended{};
  };

  template <class Promise>
  struct FinalSuspendAwaiter
  {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<Promise> handle) const noexcept
    {
      // The waiter may destroy the coroutine as soon as it is notified. Keep
      // all post-publication accesses on independently owned state, not the
      // promise/frame whose final_suspend is currently executing.
      auto state = handle.promise().final_suspend_state;
      {
        std::lock_guard lock(state->mutex);
        state->suspended = true;
      }
      state->cv.notify_all();
    }
    void await_resume() const noexcept {}
  };

  template <class Promise>
  void destroy_after_final_suspend(std::coroutine_handle<Promise> &handle) noexcept
  {
    if (!handle)
      return;
    auto state = handle.promise().final_suspend_state;
    std::unique_lock lock(state->mutex);
    const bool final = state->cv.wait_for(
        lock, std::chrono::seconds(5), [&] { return state->suspended; });
    if (!final)
      std::terminate();
    auto owned = std::exchange(handle, {});
    lock.unlock();
    owned.destroy();
  }
}
