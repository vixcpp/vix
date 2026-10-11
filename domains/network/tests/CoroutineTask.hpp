#pragma once

#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <utility>

namespace vix::network::test_detail
{
  template <class Promise>
  struct FinalSuspendAwaiter
  {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<Promise> handle) const noexcept
    {
      auto &promise = handle.promise();
      {
        std::lock_guard lock(promise.final_mutex);
        promise.final_suspended = true;
      }
      promise.final_cv.notify_all();
    }
    void await_resume() const noexcept {}
  };

  template <class Promise>
  void destroy_after_final_suspend(std::coroutine_handle<Promise> &handle) noexcept
  {
    if (!handle)
      return;
    auto &promise = handle.promise();
    std::unique_lock lock(promise.final_mutex);
    const bool final = promise.final_cv.wait_for(
        lock, std::chrono::seconds(5), [&] { return promise.final_suspended; });
    if (!final)
      std::terminate();
    auto owned = std::exchange(handle, {});
    lock.unlock();
    owned.destroy();
  }
}
