
/**
 * @file cancel_deadline.cpp
 * @brief Demonstrates pending-task cancellation, cooperative stop, and deadlines.
 *
 * Cancellation does not forcibly interrupt a running callable.
 * An already-expired deadline rejects submission before admission.
 */

#include <vix/concurrency.hpp>

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>

namespace
{
  // A gate keeps the sole worker busy so another task stays pending.
  struct Gate
  {
    std::mutex mutex;
    std::condition_variable changed;
    bool started = false;
    bool released = false;

    void worker_wait()
    {
      std::unique_lock lock(mutex);
      started = true;
      changed.notify_all();
      changed.wait(lock, [this]
                   { return released; });
    }

    void wait_until_started()
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [this]
                   { return started; });
    }

    void release()
    {
      {
        std::lock_guard lock(mutex);
        released = true;
      }
      changed.notify_all();
    }
  };
} // namespace

int main()
{
  using namespace vix::concurrency;

  // One running task and at most one queued task.
  auto created = ExecutionResource::create({1, 1});
  if (!created)
  {
    std::cerr << "Resource creation failed: " << created.error().message() << '\n';
    return 1;
  }
  auto resource = created.take();

  // Expiration is checked before the callable is admitted.
  auto expired = resource.submit(
      WorkOptions{vix::Deadline::at(vix::MonotonicClock::now())},
      []
      { return 99; });
  if (expired || expired.error().identity() != errors::deadline_expired)
  {
    std::cerr << "Expected an expired deadline to reject submission.\n";
    return 1;
  }
  std::cout << "Expired deadline: task not admitted.\n";

  auto gate = std::make_shared<Gate>();
  auto running_result = resource.submit([gate](WorkContext &context)
                                        {
    gate->worker_wait();
    // Cancellation is a request. The callable must cooperate to observe it.
    return context.stop_requested()
               ? vix::Completion<int>::cancelled()
               : vix::Completion<int>(42); });
  if (!running_result)
  {
    std::cerr << "Could not submit running work.\n";
    return 1;
  }
  auto running = running_result.take();
  gate->wait_until_started();

  // The sole worker is occupied, so this task remains pending.
  auto pending_result = resource.submit([]
                                        { return 7; });
  if (!pending_result)
  {
    gate->release();
    std::cerr << "Could not submit pending work.\n";
    return 1;
  }
  auto pending = pending_result.take();

  const bool cancelled_pending = pending.request_cancel();
  const bool cancelled_running = running.request_cancel();
  gate->release(); // Let the running callable finish after seeing the request.

  auto pending_completion = pending.take_completion();
  auto running_completion = running.take_completion();
  if (!cancelled_pending || !cancelled_running ||
      !pending_completion || !running_completion ||
      pending_completion.value().status() != vix::CompletionStatus::Cancelled ||
      running_completion.value().status() != vix::CompletionStatus::Cancelled)
  {
    std::cerr << "Unexpected cancellation result.\n";
    return 1;
  }

  std::cout << "Pending task: cancelled without running.\n";
  std::cout << "Running task: observed cancellation cooperatively.\n";

  if (!resource.shutdown())
  {
    std::cerr << "Resource shutdown failed.\n";
    return 1;
  }
  return 0;
}
