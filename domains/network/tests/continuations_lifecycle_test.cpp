#include "Engine.hpp"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <string_view>
#include <thread>
#include <vector>

using vix::network::detail::Continuations;

namespace
{
  std::atomic<bool> exit_task_completed{};
  void verify_exit_drain()
  {
    if (!exit_task_completed.load(std::memory_order_acquire))
      std::abort();
  }
}

int main(int argc, char **argv)
{
  if (argc != 2)
    return 2;

  if (std::string_view(argv[1]) == "drain-on-process-exit")
  {
    // Register the verifier before the function-local pool is initialized.
    // Its exit callback must observe the pool destructor joining workers and
    // draining a continuation deliberately held until shutdown.
    if (std::atexit(verify_exit_drain) != 0 || !Continuations::ensure())
      return 7;
    Continuations::hold_for_test(true);
    Continuations::enqueue([] { exit_task_completed.store(true, std::memory_order_release); });
    return 0;
  }
  else if (std::string_view(argv[1]) == "partial-startup")
  {
    Continuations::fail_worker_creation_after_for_test(1);
    const auto failed = Continuations::ensure();
    if (failed)
      return 3;
    const auto recovered = Continuations::ensure();
    if (!recovered)
      return 4;
  }
  else if (std::string_view(argv[1]) == "concurrent-startup")
  {
    constexpr unsigned count = 16;
    std::atomic<bool> start{};
    std::atomic<unsigned> successes{};
    std::vector<std::thread> callers;
    callers.reserve(count);
    for (unsigned i = 0; i < count; ++i)
      callers.emplace_back([&]
                           {
                             while (!start.load(std::memory_order_acquire))
                               std::this_thread::yield();
                             if (Continuations::ensure())
                               ++successes;
                           });
    start.store(true, std::memory_order_release);
    for (auto &caller : callers)
      caller.join();
    if (successes != count)
      return 5;
  }
  else
    return 2;

  std::promise<void> completed;
  auto result = completed.get_future();
  Continuations::enqueue([&completed] { completed.set_value(); });
  if (result.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
    return 6;
  return 0;
}
